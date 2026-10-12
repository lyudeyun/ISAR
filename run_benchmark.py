import argparse
import glob
import os
import shutil
import subprocess
import sys


ISAR_DIR = os.path.dirname(os.path.abspath(__file__))

# Benchmarks simulated in Python: the controller, the 100-case test suite and the 10000-case validation suite of each
PYTHON_BENCHMARKS = {
    'MC': {'benchmark': 'mc',
           'network': os.path.join('controllers', 'mc_sig_2x16_broken_old.yml'),
           'mat_path': os.path.join('benchmark_contrrep', 'MC', 'MC_Phi_1_FFNN_16_16_XV_10_10_20250609.mat'),
           'test_set': os.path.join('benchmark_contrrep', 'MC', 'mc_sig_2x16_broken_cp_testCaseNum_10000_val.mat')},
    'UUV': {'benchmark': 'uuv',
            'network': os.path.join('controllers', 'uuv_tanh_2_15_2x32_broken.yml'),
            'mat_path': os.path.join('benchmark_contrrep', 'UUV', 'UUV_Phi_1_FFNN_32_32_YH_10_10_May_13.mat'),
            'test_set': os.path.join('benchmark_contrrep', 'UUV', 'uuv_tanh_2_15_2x32_broken_cp_testCaseNum_10000_val.mat')},
}


# Results of a benchmark, grouped by benchmark folder, e.g. benchmark_opera/ACC1 -> <results_dir>/opera/ACC1;
# MC and UUV belong to benchmark_contrrep (<results_dir>/contrrep/MC)
def benchmark_results_dir(results_dir, benchmark_dir, benchmark):
    group = os.path.basename(os.path.normpath(benchmark_dir)).replace('benchmark_', '', 1)
    return os.path.abspath(os.path.join(results_dir, group, benchmark))


# The single config_rfFR* of a Simulink benchmark
def benchmark_config(benchmark_dir, benchmark):
    configs = sorted(glob.glob(os.path.join(ISAR_DIR, benchmark_dir, benchmark, 'config_rfFR*')))
    assert len(configs) == 1, f'expected one config_rfFR* in {benchmark_dir}/{benchmark}, found {configs}'
    return configs[0]


# 1-based test suite numbers from e.g. "2", "1,3" or "1-5"; all of them if spec is None
def parse_test_suites(spec, num_test_suites):
    if spec is None:
        return list(range(1, num_test_suites + 1))
    test_suites = []
    for part in str(spec).split(','):
        lo, _, hi = part.partition('-')
        test_suites += list(range(int(lo), int(hi or lo) + 1))
    test_suites = sorted(set(test_suites))
    assert test_suites[0] >= 1 and test_suites[-1] <= num_test_suites, \
        f'--test_suites must be within 1-{num_test_suites}, got {spec}'
    return test_suites


# Repeated ISAR repairs of one benchmark on its 100-case test suite, without the verifier.
# Prepares the inputs once (robustness of the test suite under the original controller), then runs
# one repair per seed, each in a fresh process (for Simulink benchmarks, with its own MATLAB session).
# Results go to <results_dir>/contrrep/<benchmark>/run_<seed>/ (<results_dir>/opera/... for benchmark_opera);
# seeds whose repair already finished are skipped.
# A Simulink config listing several test suites (benchmark_opera/) repeats this on each of them: the inputs
# of test suite <k> are prepared in <results_dir>/opera/<benchmark>/testsuite_<k>/, and its runs go to
# <results_dir>/opera/<benchmark>/testsuite_<k>/run_<seed>/.
if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("--benchmark", help="MC, UUV, or a Simulink benchmark folder under --benchmark_dir, e.g. AFC1_Phi1 or, with --benchmark_dir=benchmark_opera, ACC1")
    parser.add_argument("--benchmark_dir", help="folder of the Simulink benchmarks, benchmark_contrrep (default) or benchmark_opera", default='benchmark_contrrep')
    parser.add_argument("--test_suites", help="Simulink only, for a config listing several test suites: which ones to repair on, 1-based, "
                                              "e.g. 2 or 1,3 or 1-5; default all", default=None)
    parser.add_argument("--runs", help="number of repeated runs per test suite, default 10", default=10)
    parser.add_argument("--first_seed", help="seed of the first run; run i uses seed first_seed + i, default 1", default=1)
    parser.add_argument("--workers", help="Simulink only: number of MATLAB workers simulating in parallel", default=8)
    parser.add_argument("--breach_path", help="Simulink only: Breach installation", default=os.environ.get('ISAR_BREACH_PATH', os.path.join(os.path.dirname(ISAR_DIR), 'breach')))
    parser.add_argument("--results_dir", help="directory for all results", default=os.path.join(ISAR_DIR, 'results'))
    parser.add_argument("--eval_timeout", help="Simulink only: seconds to wait for one batch of simulations before restarting MATLAB, default 900", default=900)
    parser.add_argument("--std", help="standard deviation of the annealing perturbation; if given, runs go to std_<std>/run_<seed>/ instead of run_<seed>/ (default 0.1)", default=None)
    parser.add_argument("--prepare_only", help="1 to only prepare the inputs, e.g. once before submitting one job per seed", default=0)
    args = parser.parse_args()

    out_dir = benchmark_results_dir(args.results_dir, args.benchmark_dir, args.benchmark)
    num_test_suites = 1
    if args.benchmark in PYTHON_BENCHMARKS:
        bm = PYTHON_BENCHMARKS[args.benchmark]
        kind = bm['benchmark']
        prepare_args = ['--mat_path=' + bm['mat_path'], '--network=' + bm['network']]
        repair_args = ['--network=' + bm['network']]
    else:
        config = benchmark_config(args.benchmark_dir, args.benchmark)
        kind = 'simulink'
        from simulink_interface import parse_autopatch_config
        num_test_suites = len(parse_autopatch_config(config)['testSuite'])
        if not os.path.isfile(os.path.join(args.breach_path, 'InitBreach.m')):
            sys.exit(f'Breach not found in {os.path.abspath(args.breach_path)} (no InitBreach.m); '
                     f'pass the Breach folder with --breach_path or set ISAR_BREACH_PATH')
        prepare_args = ['--simulink_config=' + config, '--breach_path=' + os.path.abspath(args.breach_path),
                        '--workers=' + str(args.workers), '--eval_timeout=' + str(args.eval_timeout)]
        repair_args = prepare_args
    seeds = [int(args.first_seed) + i for i in range(int(args.runs))]

    # Test suites to repair on (None: the single test suite of the benchmark)
    if num_test_suites == 1:
        assert args.test_suites is None, f'{args.benchmark} has a single test suite, --test_suites does not apply'
        test_suites = [None]
    else:
        test_suites = parse_test_suites(args.test_suites, num_test_suites)

    def suite_dir(k):
        return out_dir if k is None else os.path.join(out_dir, f'testsuite_{k}')

    def suite_label(k):
        return args.benchmark if k is None else f'{args.benchmark} test suite {k}'

    def suite_args(k):
        return [] if k is None else ['--test_suite=' + str(k)]

    def prepared_paths(k):
        return (os.path.join(suite_dir(k), f'{kind}_verisig_result_skipped.csv'),
                os.path.join(suite_dir(k), f'{kind}_sampling_result_broken.csv'))

    # Run from the repository root: the UUV simulator reads uuv_model_oneHz.mat from the current folder
    for k in test_suites:
        verisig_path, sampled_path = prepared_paths(k)
        if not (os.path.isfile(verisig_path) and os.path.isfile(sampled_path)):
            print(f'Preparing {suite_label(k)}: robustness of the test suite under the original controller', flush=True)
            subprocess.run([sys.executable, '-u', 'testsuite_to_isar.py', '--benchmark=' + kind, '--output_dir=' + suite_dir(k)]
                           + prepare_args + suite_args(k), cwd=ISAR_DIR, check=True)
            shutil.rmtree(os.path.join(suite_dir(k), 'matlab_work'), ignore_errors=True)
    if int(args.prepare_only):
        sys.exit(0)

    for k in test_suites:
        verisig_path, sampled_path = prepared_paths(k)
        # Runs with a non-default perturbation are kept apart from the runs with the default of ISAR
        runs_dir = suite_dir(k) if args.std is None else os.path.join(suite_dir(k), f'std_{float(args.std):g}')
        for seed in seeds:
            label = f'{suite_label(k)} seed {seed}'
            run_dir = os.path.join(runs_dir, f'run_{seed}')
            log_path = os.path.join(run_dir, 'repair.log')
            if os.path.isfile(log_path) and 'Total time:' in open(log_path).read():
                print(f'{label}: already finished, skipped', flush=True)
                continue
            # A run that did not finish is started over in an empty folder, so no checkpoint of the earlier attempt remains
            if os.path.isdir(run_dir):
                print(f'{label}: removing the unfinished earlier attempt in {run_dir}', flush=True)
                shutil.rmtree(run_dir)
            os.makedirs(run_dir)
            print(f'{label}: repairing, log in {log_path}', flush=True)
            with open(log_path, 'w') as log:
                result = subprocess.run([sys.executable, '-u', 'incremental_repair.py', '--benchmark=' + kind, '--seed=' + str(seed),
                                         '--verisig_result_path=' + verisig_path, '--sampled_result_path=' + sampled_path,
                                         '--output_path=' + run_dir] + repair_args + suite_args(k)
                                        + ([] if args.std is None else ['--std=' + str(float(args.std))]),
                                        cwd=ISAR_DIR, stdout=log, stderr=subprocess.STDOUT)
            if result.returncode != 0:
                print(f'{label}: failed with exit code {result.returncode}, see {log_path}', flush=True)
                sys.exit(result.returncode)
            # The model copies made by fastRob are only needed while the run is active
            shutil.rmtree(os.path.join(run_dir, 'matlab_work'), ignore_errors=True)
            print(f'{label}: done', flush=True)
