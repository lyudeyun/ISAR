import argparse
import glob
import json
import os
import re
import shutil

import numpy as np
import pandas as pd
import scipy.io as sio
import torch
import yaml

from incremental_repair_utils import (MC_Control_NN, UUV_Control_NN, load_model_dict,  # the classes: to unpickle checkpoints
                                      mc_robustness, mc_simulate, uuv_robustness, uuv_simulate)
from run_benchmark import ISAR_DIR, PYTHON_BENCHMARKS, benchmark_config, benchmark_results_dir, parse_test_suites, python_config
from simulink_interface import DEFAULT_BREACH_PATH, SimulinkBenchmark, SimulinkNN, parse_autopatch_config  # SimulinkNN: to unpickle checkpoints


# The test set of MC or UUV, simulated in Python like during repair; same interface as SimulinkBenchmark
class PythonTestSet:

    def __init__(self, kind, network_path, test_set_path):
        self.kind = kind
        test_cases = sio.loadmat(test_set_path, squeeze_me=True, struct_as_record=False)['testSuite'].testCases
        # sysInputs is [y, h] for uuv and [pos, vel] for mc, as in testsuite_to_isar.py
        self.inputs = [(float(t.sysInputs[0]), float(t.sysInputs[1])) for t in test_cases]
        self.stored_rob = np.array([float(t.rob) for t in test_cases])
        self.num_cases = len(self.inputs)
        # The executions recorded in the test set, if any (OPERA's MC): MC_rep_eval takes the metrics before
        # repair from them
        self.recorded = None
        if 'trajPos' in test_cases[0]._fieldnames and 'nnOutputs' in test_cases[0]._fieldnames:
            self.recorded = [(np.atleast_1d(t.trajPos), np.atleast_1d(t.nnOutputs)) for t in test_cases]
        with open(network_path, 'rb') as f:
            self.model_dict = yaml.safe_load(f)

    def original_net(self):
        net = UUV_Control_NN() if self.kind == 'uuv' else MC_Control_NN()
        load_model_dict(self.model_dict, net)
        return net

    def robustness(self, net, case_ids):
        rob = []
        for c in case_ids:
            v1, v2 = self.inputs[c]
            if self.kind == 'uuv':
                _, traj_pos_y = uuv_simulate(net=net, init_global_heading_deg=v2, init_pos_y=v1)
                rob.append(uuv_robustness(traj_pos_y))
            else:
                traj_pos, _ = mc_simulate(v1, v2, net)
                rob.append(mc_robustness(traj_pos))
        return rob

    # Robustness and MC's performance metrics (mc_metrics) of the test cases case_ids under the controller of the
    # yml file network_path, simulated as MC_rep_eval.m does (mc_simulate_double)
    def robustness_and_metrics(self, network_path, case_ids):
        assert self.kind == 'mc', 'performance metrics are defined for MC only'
        with open(network_path, 'rb') as f:
            model_dict = yaml.safe_load(f)
        rob, rows = [], []
        for c in case_ids:
            traj_pos, u = mc_simulate_double(self.inputs[c][0], self.inputs[c][1], model_dict)
            rob.append(mc_robustness(traj_pos))
            rows.append(mc_metrics(traj_pos, u))
        return np.array(rob), ['reach', 'energy', 'smooth'], pd.DataFrame(rows)[['reach', 'energy', 'smooth']].values

    def close(self):
        pass


# The controller a run ends with: its last checkpoint, or the original controller if no region was repaired
def final_checkpoint(run_dir):
    checkpoints = []
    for path in glob.glob(os.path.join(run_dir, 'tanh_iter_*_region_*.pt')):
        m = re.fullmatch(r'tanh_iter_(\d+)_region_(\d+)\.pt', os.path.basename(path))
        if m:
            checkpoints.append((int(m.group(1)), path))
    return max(checkpoints)[1] if checkpoints else None


# Metrics of OPERA's rep_eval: a test case is satisfied if its robustness is > 0 and violated if < 0
def compare(rob_before, rob_after):
    rob_before, rob_after = np.asarray(rob_before), np.asarray(rob_after)
    n = len(rob_before)
    return {'num_cases': n,
            'satisfied_before': int((rob_before > 0).sum()),
            'satisfied_after': int((rob_after > 0).sum()),
            'SR_before': 100.0 * (rob_before > 0).sum() / n,
            'SR_after': 100.0 * (rob_after > 0).sum() / n,
            'neg_to_pos': int(((rob_before < 0) & (rob_after > 0)).sum()),
            'pos_to_neg': int(((rob_before > 0) & (rob_after < 0)).sum())}


# OPERA's performance metrics on the test set, from the per-test-case quantities of simulink/isarOperaMetrics.m (a
# DataFrame, one column per quantity), aggregated and named as in AutoRepair_Alpha's test/rep_eval_acc.py,
# rep_eval_afc.py and rep_eval_wt.py
def opera_metrics(bench, q):
    mean = lambda name: float(np.mean(q[name]))
    ratio = lambda num, den: float(np.sum(q[num]) / np.sum(q[den])) if np.sum(q[den]) > 0 else float('nan')
    if bench == 'ACC':
        return {'te_mean_x_ego': mean('x_ego'), 'te_mean_steady': mean('steady'),
                'te_mean_delta_v': ratio('delta_v_sum', 'delta_v_time'), 'te_mean_comfortable': mean('comfortable'),
                'te_mean_resilience': ratio('recovery', 'unsteady')}
    if bench == 'AFC':
        return {'te_mean_steady': mean('steady'), 'te_mean_recovery': ratio('recovery', 'unsteady'),
                'te_mean_MAE': mean('MAE'), 'te_mean_MAXERR': mean('MAXERR')}
    if bench == 'WT':
        return {'te_mean_steady': mean('steady'), 'te_mean_MAE': mean('MAE'), 'te_mean_MAXERR': mean('MAXERR')}
    return {'te_mean_reach': mean('reach'), 'te_mean_energy': mean('energy'), 'te_mean_smooth': mean('smooth')}


# Names of the per-test-case quantities in the files, as the te_*_set variables of rep_eval and MC_rep_eval
OPERA_SET_NAMES = {'ACC': {'x_ego': 'te_x_ego_set', 'steady': 'te_steady_num_set', 'delta_v_sum': 'te_delta_v_sum_set',
                           'delta_v_time': 'te_delta_v_time_set', 'comfortable': 'te_comfortable_set',
                           'unsteady': 'te_unsteady_num_set', 'recovery': 'te_recovery_num_set'},
                   'AFC': {'steady': 'te_steady_set', 'unsteady': 'te_unsteady_set', 'recovery': 'te_recovery_set',
                           'MAE': 'te_MAE_set', 'MAXERR': 'te_MAXERR_set'},
                   'WT': {'steady': 'te_steady_set', 'MAE': 'te_MAE_set', 'MAXERR': 'te_MAXERR_set'},
                   'MC': {'reach': 'te_reach_set', 'energy': 'te_energy_set', 'smooth': 'te_smooth_set'}}


# MC's performance metrics of one execution, as mcMetrics in AutoRepair_Alpha's src/eval/MC_rep_eval.m: reach is the
# step at which the position first reaches 0.45 (the horizon if never), energy and smooth the sum of u^2 and of
# |u(t) - u(t-1)| up to that step; traj_pos[0] is the initial state and u[t] the control applied after traj_pos[t]
def mc_metrics(traj_pos, u):
    horizon = len(u)
    reached = np.nonzero(np.asarray(traj_pos[1:]) >= 0.45)[0]
    k = horizon if len(reached) == 0 else int(reached[0]) + 1
    uk = np.asarray(u[:k], dtype=float)
    return {'reach': k, 'energy': float(np.sum(uk ** 2)), 'smooth': float(np.sum(np.abs(np.diff(uk))))}


# MC simulated as simMatlabMC.m of AutoRepair_Alpha (benchmark/mc), which MC_rep_eval.m uses: the controller in
# double precision with the weights of its yml file. mc_simulate, used during repair, has the same dynamics but
# evaluates the network in single precision, which moves the metrics by about 1e-7. Returns the positions (initial
# state first) and the controls applied after each of them.
def mc_simulate_double(pos_0, vel_0, model_dict, length=111, steepness=0.0025):
    W = [np.array(model_dict['weights'][l], dtype=float) for l in (1, 2, 3)]
    b = [np.array(model_dict['offsets'][l], dtype=float).ravel() for l in (1, 2, 3)]
    sigmoid = lambda x: 1 / (1 + np.exp(-x))
    traj_pos, traj_vel, u = np.zeros(length), np.zeros(length), np.zeros(length - 1)
    traj_pos[0], traj_vel[0] = pos_0, vel_0
    for i in range(1, length):
        traj_pos[i] = traj_pos[i - 1] + traj_vel[i - 1]
        x = np.array([traj_pos[i - 1], traj_vel[i - 1]])
        nn_out = np.tanh(W[2] @ sigmoid(W[1] @ sigmoid(W[0] @ x + b[0]) + b[1]) + b[2])[0]
        u[i - 1] = nn_out
        traj_vel[i] = traj_vel[i - 1] + 0.0015 * nn_out - steepness * np.cos(3 * traj_pos[i - 1])
        traj_vel[i] = min(max(traj_vel[i], -0.07), 0.07)
        traj_pos[i] = min(max(traj_pos[i], -1.2), 0.6)
        if traj_pos[i] == -1.2 and traj_vel[i] < 0:
            traj_vel[i] = 0
    return traj_pos, u


# Evaluates the controllers repaired by run_benchmark.py on the test set of a benchmark, as OPERA's rep_eval
# and AutoPatch's validatePatch do: the testSet entry of a Simulink config (the 1000-case OPERA test set, the
# 10000-case validation suite of the ContrRep benchmarks), or the validation suite of MC and UUV.
# The original controller is simulated once, in <results>/testset/original.csv; each finished run gets
# testset_robustness.csv and testset_result.json in its run folder, and all runs are summarized in
# <results>/testset/summary.csv. Runs that are already evaluated are skipped, runs that have not finished
# yet are left out, so the script can be called again while repairs are still running.
if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("--benchmark", help="MC_old, UUV, or a benchmark folder under --benchmark_dir, e.g. ACC1, MC_new or AFC1_Phi1")
    parser.add_argument("--benchmark_dir", help="folder of the Simulink benchmarks and MC_new, benchmark_opera (default) or "
                                                "benchmark_contrrep; not needed for MC_old and UUV", default='benchmark_opera')
    parser.add_argument("--test_suites", help="for a config listing several test suites: the runs of which ones to evaluate, "
                                              "e.g. 2 or 1,3 or 1-5; default all", default=None)
    parser.add_argument("--runs", help="number of runs per test suite, default 10", default=10)
    parser.add_argument("--first_seed", help="seed of the first run, default 1", default=1)
    parser.add_argument("--std", help="evaluate the runs with this perturbation std (std_<std>/run_<seed>/) instead of the default", default=None)
    parser.add_argument("--workers", help="number of MATLAB workers simulating in parallel", default=8)
    parser.add_argument("--breach_path", help="Breach installation", default=DEFAULT_BREACH_PATH)
    parser.add_argument("--results_dir", help="directory for all results, as for run_benchmark.py", default=os.path.join(ISAR_DIR, 'results'))
    parser.add_argument("--eval_timeout", help="seconds to wait for one batch of simulations before restarting MATLAB, default 900", default=900)
    args = parser.parse_args()

    python_bm = None
    if args.benchmark in PYTHON_BENCHMARKS:
        args.benchmark_dir = 'benchmark_contrrep'
        python_bm = PYTHON_BENCHMARKS[args.benchmark]
        num_test_suites = 1
    else:
        config_path = benchmark_config(args.benchmark_dir, args.benchmark)
        config = parse_autopatch_config(config_path)
        assert 'testSet' in config, f'{config_path} has no testSet entry'
        num_test_suites = len(config['testSuite'])
        # MC or UUV with its own controller and test set in the benchmark folder (benchmark_opera/MC_new)
        python_bm = python_config(config_path)
    test_set = os.path.basename(python_bm['test_set']) if python_bm is not None else config['testSet'][0]
    out_dir = benchmark_results_dir(args.results_dir, args.benchmark_dir, args.benchmark)
    # The UUV simulator reads uuv_model_oneHz.mat from the current folder
    os.chdir(ISAR_DIR)
    test_suites = [None] if num_test_suites == 1 else parse_test_suites(args.test_suites, num_test_suites)
    seeds = [int(args.first_seed) + i for i in range(int(args.runs))]
    eval_dir = os.path.join(out_dir, 'testset' if args.std is None else f'testset_std_{float(args.std):g}')
    os.makedirs(eval_dir, exist_ok=True)

    # The runs to evaluate, as laid out by run_benchmark.py
    runs = []
    for k in test_suites:
        suite_dir = out_dir if k is None else os.path.join(out_dir, f'testsuite_{k}')
        runs_dir = suite_dir if args.std is None else os.path.join(suite_dir, f'std_{float(args.std):g}')
        for seed in seeds:
            runs.append((k, seed, os.path.join(runs_dir, f'run_{seed}')))

    def finished(run_dir):
        log_path = os.path.join(run_dir, 'repair.log')
        return os.path.isfile(log_path) and 'Total time:' in open(log_path).read()

    def label(k, seed):
        return f'{args.benchmark} seed {seed}' if k is None else f'{args.benchmark} test suite {k} seed {seed}'

    original_path = os.path.join(eval_dir, 'original.csv')
    todo = [(k, seed, run_dir) for k, seed, run_dir in runs
            if finished(run_dir) and not os.path.isfile(os.path.join(run_dir, 'testset_result.json'))]
    for k, seed, run_dir in runs:
        if not finished(run_dir):
            print(f'{label(k, seed)}: repair not finished, skipped', flush=True)

    if todo or not os.path.isfile(original_path):
        if python_bm is not None:
            bench = PythonTestSet(python_bm['benchmark'], python_bm['network'], python_bm['test_set'])
        else:
            bench = SimulinkBenchmark(os.path.dirname(config_path), config_path, args.breach_path,
                                      os.path.join(eval_dir, 'matlab_work'), workers=int(args.workers),
                                      eval_timeout=float(args.eval_timeout), test_suite_file=test_set)
        try:
            case_ids = list(range(bench.num_cases))
            if not os.path.isfile(original_path):
                print(f'{args.benchmark}: simulating the {bench.num_cases} test cases of {test_set} under the original controller', flush=True)
                rob = np.array(bench.robustness(bench.original_net(), case_ids))
                stored = bench.stored_rob
                print(f'Recomputed robustness vs. rob in the test set: max |diff| = {np.max(np.abs(rob - stored)):.2e}, '
                      f'sign mismatches = {int(np.sum((rob < 0) != (stored < 0)))}', flush=True)
                df = pd.DataFrame({'case': case_ids, 'rob_stored': stored, 'rob': rob})
                df.to_csv(original_path + '.tmp', index=False)
                os.replace(original_path + '.tmp', original_path)
            rob_original = pd.read_csv(original_path)['rob'].values

            for k, seed, run_dir in todo:
                checkpoint = final_checkpoint(run_dir)
                if checkpoint is None:
                    net, rob = None, rob_original
                else:
                    net = torch.load(checkpoint, weights_only=False)
                    rob = np.array(bench.robustness(net, case_ids))
                result = compare(rob_original, rob)
                result.update({'test_set': test_set,
                               'checkpoint': None if checkpoint is None else os.path.basename(checkpoint)})
                pd.DataFrame({'case': case_ids, 'rob_original': rob_original, 'rob_repaired': rob}).to_csv(
                    os.path.join(run_dir, 'testset_robustness.csv'), index=False)
                with open(os.path.join(run_dir, 'testset_result.json.tmp'), 'w') as f:
                    json.dump(result, f, indent=2)
                os.replace(os.path.join(run_dir, 'testset_result.json.tmp'), os.path.join(run_dir, 'testset_result.json'))
                print(f'{label(k, seed)}: SR on the test set {result["SR_before"]:.1f} -> {result["SR_after"]:.1f}, '
                      f'repaired {result["neg_to_pos"]}, broken {result["pos_to_neg"]}'
                      + ('' if checkpoint is None else f' ({result["checkpoint"]})'), flush=True)
        finally:
            bench.close()
            shutil.rmtree(os.path.join(eval_dir, 'matlab_work'), ignore_errors=True)

    # Summary of all evaluated runs
    rows = []
    for k, seed, run_dir in runs:
        result_path = os.path.join(run_dir, 'testset_result.json')
        if os.path.isfile(result_path):
            with open(result_path) as f:
                rows.append({'test_suite': k, 'seed': seed, **json.load(f)})
    if rows:
        summary = pd.DataFrame(rows)
        summary.to_csv(os.path.join(eval_dir, 'summary.csv'), index=False)
        sr = summary['SR_after']
        print(f'{args.benchmark}: {len(summary)} runs evaluated; SR on the test set {summary["SR_before"].iloc[0]:.1f} -> '
              f'{sr.mean():.2f} ± {sr.std(ddof=1) if len(sr) > 1 else 0.0:.2f} (mean ± std over runs); '
              f'repaired {summary["neg_to_pos"].mean():.1f}, broken {summary["pos_to_neg"].mean():.1f} on average')
        print(f'Summary in {os.path.join(eval_dir, "summary.csv")}')

    # OPERA's performance metrics on the test set, for benchmark_opera (ACC, AFC, WT as in rep_eval_*.py, MC_new as in
    # MC_rep_eval.m): testset_metrics.json (the te_mean_* metrics) and testset_metrics_cases.csv (their per-test-case
    # quantities, the te_*_set) in each run folder, original_metrics.json, original_metrics_cases.csv and
    # metrics_summary.csv in the test-set folder. Computed in a simulation session of their own, which records the extra signals they need; the
    # robustness and the files above are left as they are. Runs that already have them, or whose repair has not
    # finished, are skipped.
    metrics_kind = None
    if os.path.basename(os.path.normpath(args.benchmark_dir)) == 'benchmark_opera':
        if python_bm is not None:
            metrics_kind = 'MC' if python_bm['benchmark'] == 'mc' else None
        elif config['bench'][0] in ('ACC', 'AFC', 'WT'):
            metrics_kind = config['bench'][0]
    if metrics_kind is not None:
        original_metrics_path = os.path.join(eval_dir, 'original_metrics.json')
        original_cases_path = os.path.join(eval_dir, 'original_metrics_cases.csv')
        todo = [(k, seed, run_dir) for k, seed, run_dir in runs
                if finished(run_dir) and not (os.path.isfile(os.path.join(run_dir, 'testset_metrics.json'))
                                              and os.path.isfile(os.path.join(run_dir, 'testset_metrics_cases.csv')))]

        def write_cases(path, q):
            cases = q.rename(columns=OPERA_SET_NAMES[metrics_kind])
            cases.insert(0, 'case', range(len(cases)))
            cases.to_csv(path + '.tmp', index=False)
            os.replace(path + '.tmp', path)

        def write_json(path, data):
            with open(path + '.tmp', 'w') as f:
                json.dump(data, f, indent=2)
            os.replace(path + '.tmp', path)

        if todo or not (os.path.isfile(original_metrics_path) and os.path.isfile(original_cases_path)):
            if python_bm is not None:
                bench = PythonTestSet(python_bm['benchmark'], python_bm['network'], python_bm['test_set'])
            else:
                bench = SimulinkBenchmark(os.path.dirname(config_path), config_path, args.breach_path,
                                          os.path.join(eval_dir, 'matlab_work'), workers=int(args.workers),
                                          eval_timeout=float(args.eval_timeout), test_suite_file=test_set, metrics=metrics_kind)
            try:
                case_ids = list(range(bench.num_cases))

                # Metrics of a controller, and the largest difference of its robustness to the one computed above;
                # MC is simulated from the controller's yml file (the checkpoint's twin), the others from the network
                def measure(checkpoint, rob_before):
                    if python_bm is not None:
                        target = python_bm['network'] if checkpoint is None else checkpoint[:-len('.pt')] + '.yml'
                    else:
                        target = bench.original_net() if checkpoint is None else torch.load(checkpoint, weights_only=False)
                    rob, names, quantities = bench.robustness_and_metrics(target, case_ids)
                    q = pd.DataFrame(quantities, columns=names)
                    return opera_metrics(metrics_kind, q), q, float(np.max(np.abs(rob - rob_before)))

                if not (os.path.isfile(original_metrics_path) and os.path.isfile(original_cases_path)):
                    print(f'{args.benchmark}: performance metrics of the original controller on {test_set}', flush=True)
                    m, q, diff = measure(None, pd.read_csv(original_path)['rob'].values)
                    write_cases(original_cases_path, q)
                    print(f'  robustness vs. original.csv: max |diff| = {diff:.2e}', flush=True)
                    if metrics_kind == 'MC' and bench.recorded is not None:
                        # MC_rep_eval takes the metrics before repair from the executions recorded in the test set
                        recorded = opera_metrics('MC', pd.DataFrame([mc_metrics(p, u) for p, u in bench.recorded]))
                        print('  vs. the executions recorded in the test set: '
                              + ', '.join(f'{n} {recorded[n]:.10g} (|diff| {abs(recorded[n] - m[n]):.2e})' for n in m), flush=True)
                    print('  ' + ', '.join(f'{n} {v:.10g}' for n, v in m.items()), flush=True)
                    write_json(original_metrics_path, {'test_set': test_set, 'checkpoint': None, **m})
                with open(original_metrics_path) as f:
                    original_metrics = {n: v for n, v in json.load(f).items() if n.startswith('te_mean_')}

                for k, seed, run_dir in todo:
                    checkpoint = final_checkpoint(run_dir)
                    if checkpoint is None:
                        m = original_metrics
                        shutil.copyfile(original_cases_path, os.path.join(run_dir, 'testset_metrics_cases.csv'))
                    else:
                        rob_path = os.path.join(run_dir, 'testset_robustness.csv')
                        rob_before = pd.read_csv(rob_path)['rob_repaired'].values if os.path.isfile(rob_path) else np.zeros(bench.num_cases)
                        m, q, diff = measure(checkpoint, rob_before)
                        write_cases(os.path.join(run_dir, 'testset_metrics_cases.csv'), q)
                        if os.path.isfile(rob_path) and diff > 0:
                            print(f'  {label(k, seed)}: robustness differs from testset_robustness.csv by up to {diff:.2e}', flush=True)
                    write_json(os.path.join(run_dir, 'testset_metrics.json'),
                               {'test_set': test_set, 'checkpoint': None if checkpoint is None else os.path.basename(checkpoint), **m})
                    print(f'{label(k, seed)}: ' + ', '.join(f'{n[len("te_mean_"):]} {original_metrics[n]:.4g} -> {v:.4g}'
                                                           for n, v in m.items()), flush=True)
            finally:
                bench.close()
                shutil.rmtree(os.path.join(eval_dir, 'matlab_work'), ignore_errors=True)

        rows = []
        for k, seed, run_dir in runs:
            path = os.path.join(run_dir, 'testset_metrics.json')
            if os.path.isfile(path):
                with open(path) as f:
                    rows.append({'test_suite': k, 'seed': seed, **json.load(f)})
        if rows:
            summary = pd.DataFrame(rows)
            summary.to_csv(os.path.join(eval_dir, 'metrics_summary.csv'), index=False)
            with open(original_metrics_path) as f:
                original_metrics = json.load(f)
            print(f'{args.benchmark}: performance metrics on the test set, original -> mean ± std over {len(summary)} runs')
            for n in [c for c in summary.columns if c.startswith('te_mean_')]:
                v = summary[n]
                print(f'  {n}: {original_metrics[n]:.6g} -> {v.mean():.6g} ± {v.std(ddof=1) if len(v) > 1 else 0.0:.4g}')
            print(f'Summary in {os.path.join(eval_dir, "metrics_summary.csv")}')
