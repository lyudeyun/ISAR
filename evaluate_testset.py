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
