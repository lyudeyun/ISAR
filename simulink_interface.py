import os
import glob
import signal
import subprocess
import numpy as np
import torch
import torch.nn as nn


ISAR_DIR = os.path.dirname(os.path.abspath(__file__))

# Breach: the environment variable ISAR_BREACH_PATH if set, otherwise a folder named breach next to this repository
DEFAULT_BREACH_PATH = os.environ.get('ISAR_BREACH_PATH', os.path.join(os.path.dirname(ISAR_DIR), 'breach'))


def check_breach_path(breach_path):
    if not os.path.isfile(os.path.join(breach_path, 'InitBreach.m')):
        raise FileNotFoundError(f'Breach not found in {os.path.abspath(breach_path)} (no InitBreach.m); '
                                f'pass the Breach folder with --breach_path or set ISAR_BREACH_PATH')


# Controller of a Simulink benchmark. It only holds the weights and biases; the network is
# evaluated inside the Simulink model. Weights and biases are parameters, so the simulated
# annealing perturbs them exactly like the PyTorch controllers of UUV and MC.
class SimulinkNN(nn.Module):

    def __init__(self, weights, biases, perturb_biases=True):
        super(SimulinkNN, self).__init__()
        self.weights = nn.ParameterList([nn.Parameter(torch.tensor(w, dtype=torch.float64)) for w in weights])
        if perturb_biases:
            self.biases = nn.ParameterList([nn.Parameter(torch.tensor(b, dtype=torch.float64)) for b in biases])
        else:
            self.biases = [torch.tensor(b, dtype=torch.float64) for b in biases]

    def flat_weights(self):
        # Column-major per layer, layers concatenated, as expected by isar_sl.m
        return np.concatenate([w.detach().numpy().flatten(order='F') for w in self.weights])

    def flat_biases(self):
        return np.concatenate([b.detach().numpy().flatten(order='F') for b in self.biases])


# Read an AutoPatch config file: each entry is a line "key N" followed by N value lines
def parse_autopatch_config(path):
    with open(path) as f:
        lines = [line.rstrip('\n') for line in f]
    config, i = {}, 0
    while i < len(lines):
        parts = lines[i].split()
        if len(parts) == 2 and parts[1].isdigit():
            n = int(parts[1])
            config[parts[0]] = [line.strip() for line in lines[i + 1:i + 1 + n]]
            i += 1 + n
        else:
            i += 1
    return config


# Kill a process and all its descendants
def kill_process_tree(pid):
    pids, todo = [], [int(pid)]
    while todo:
        p = todo.pop()
        pids.append(p)
        children = subprocess.run(['pgrep', '-P', str(p)], capture_output=True, text=True).stdout.split()
        todo += [int(c) for c in children]
    for p in pids:
        try:
            os.kill(p, signal.SIGKILL)
        except (ProcessLookupError, PermissionError):
            pass


# Interface to one Simulink benchmark through the MATLAB Engine.
# MATLAB R2021a can deadlock inside sim() (in the Simulation Data Inspector), which would hang the repair
# forever. Every call to MATLAB therefore has a timeout; on a timeout, MATLAB and its workers are killed
# and restarted, and the same call is made again. Simulation is deterministic, so the result is unchanged.
class SimulinkBenchmark:

    def __init__(self, bench_dir, config_path, breach_path, work_dir, workers=1, perturb_biases=True,
                 eval_timeout=900, init_timeout=1800, max_retries=3, test_suite=1):
        check_breach_path(breach_path)
        import matlab
        import matlab.engine

        config = parse_autopatch_config(config_path)
        bench_dir = os.path.abspath(bench_dir)
        # The model, network and test suites sit in the single model folder of the benchmark
        model_dirs = [d for d in glob.glob(os.path.join(bench_dir, '*')) if os.path.isdir(d)]
        assert len(model_dirs) == 1, f'expected one model folder in {bench_dir}, found {model_dirs}'
        model_dir = model_dirs[0]

        # The config line is "<name> <formula>"; the formula is everything after the name
        phi_str = config['phi_str'][0].split(None, 1)[1]
        in_span = [float(x) for x in config['in_span'][0].split()]
        nn_stru = [float(x) for x in config['nnStru'][0].strip('[]').split(',')]

        self.perturb_biases = perturb_biases
        self.matlab = matlab
        self.eval_timeout = float(eval_timeout)
        self.init_timeout = float(init_timeout)
        self.max_retries = int(max_retries)
        self.restarts = 0
        self.eng = None
        self.matlab_pids = []
        self.cfg = {
            'benchDir': model_dir,
            'model': config['model'][0],
            'nnFile': config['nn'][0],
            'nnStru': matlab.double(nn_stru),
            'phiStr': phi_str,
            'params': config['parameters'],
            'inName': config['in_name'],
            'inSpan': matlab.double(in_span),
            # A config may list several test suites (one per repeated run); test_suite is 1-based
            'testSuiteFile': config['testSuite'][int(test_suite) - 1],
            'normFile': os.path.join(bench_dir, 'norm_params.mat'),
            'breachPath': os.path.abspath(breach_path),
            'workDir': os.path.abspath(work_dir),
            'workers': float(workers),
        }
        info = self._call('init', self.cfg)

        self.w_shapes = [tuple(int(x) for x in row) for row in np.asarray(info['Wshape'])]
        self.b_shapes = [tuple(int(x) for x in row) for row in np.asarray(info['bshape'])]
        self.weights = self._unflatten(np.asarray(info['Wflat']).ravel(), self.w_shapes)
        self.biases = self._unflatten(np.asarray(info['bflat']).ravel(), self.b_shapes)
        self.num_cases = int(info['numCases'])
        self.stored_rob = np.asarray(info['storedRob']).ravel()

    # Start MATLAB and set up the benchmark, remembering the processes to kill on a hang
    def _start(self):
        self.eng = self.matlab.engine.start_matlab(background=True).result(timeout=self.init_timeout)
        self.matlab_pids = [int(self.eng.feature('getpid'))]
        self.eng.addpath(os.path.join(ISAR_DIR, 'simulink'), nargout=0)
        info = self.eng.isar_sl('init', self.cfg, nargout=1, background=True).result(timeout=self.init_timeout)
        self.matlab_pids += [int(p) for p in np.asarray(info['workerPids']).ravel()]
        return info

    def _kill(self):
        for pid in self.matlab_pids:
            kill_process_tree(pid)
        self.eng = None
        self.matlab_pids = []

    # Call isar_sl(*args) with a timeout, restarting MATLAB and retrying on a hang or a dead engine
    def _call(self, *args):
        engine = self.matlab.engine
        retryable = (engine.TimeoutError, TimeoutError, engine.EngineError)
        last_error = None
        for attempt in range(self.max_retries + 1):
            try:
                if self.eng is None:
                    info = self._start()
                    if args[0] == 'init':
                        return info
                elif args[0] == 'init':
                    return self.eng.isar_sl(*args, nargout=1, background=True).result(timeout=self.init_timeout)
                return self.eng.isar_sl(*args, nargout=1, background=True).result(timeout=self.eval_timeout)
            except retryable as error:
                last_error = error
                print(f'MATLAB call {args[0]!r} failed ({type(error).__name__}: {error}); '
                      f'killing MATLAB processes {self.matlab_pids} and restarting '
                      f'(retry {attempt + 1} of {self.max_retries})', flush=True)
                self._kill()
                self.restarts += 1
        raise RuntimeError(f'MATLAB call {args[0]!r} failed after {self.max_retries} restarts') from last_error

    @staticmethod
    def _unflatten(v, shapes):
        out, k = [], 0
        for shape in shapes:
            n = int(np.prod(shape))
            out.append(v[k:k + n].reshape(shape, order='F'))
            k += n
        assert k == len(v)
        return out

    def original_net(self):
        return SimulinkNN(self.weights, self.biases, self.perturb_biases)

    # STL robustness of the test cases case_ids (0-based) under the controller net
    def robustness(self, net, case_ids):
        case_ids = [int(c) for c in case_ids]
        if len(case_ids) == 0:
            return []
        rob = self._call('eval',
                         self.matlab.double(net.flat_weights().tolist()),
                         self.matlab.double(net.flat_biases().tolist()),
                         self.matlab.double([c + 1 for c in case_ids]))
        return list(np.asarray(rob, dtype=float).ravel())

    # Weights and biases currently in the model blocks, to check them against the network file
    def read_model_params(self):
        p = self._call('read_model_params')
        return (self._unflatten(np.asarray(p['Wflat']).ravel(), self.w_shapes),
                self._unflatten(np.asarray(p['bflat']).ravel(), self.b_shapes))

    def close(self):
        if self.eng is None:
            return
        try:
            self.eng.isar_sl('close', nargout=0, background=True).result(timeout=self.eval_timeout)
            self.eng.quit()
        except Exception:
            self._kill()
