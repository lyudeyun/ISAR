import argparse
import glob
import os
import shutil
import subprocess
import sys


ISAR_DIR = os.path.dirname(os.path.abspath(__file__))

# Benchmarks simulated in Python: the controller and the 100-case test suite of each
PYTHON_BENCHMARKS = {
    'MC': {'benchmark': 'mc',
           'network': os.path.join('controllers', 'mc_sig_2x16_broken_old.yml'),
           'mat_path': os.path.join('benchmark_contrrep', 'MC', 'MC_Phi_1_FFNN_16_16_XV_10_10_20250609.mat')},
    'UUV': {'benchmark': 'uuv',
            'network': os.path.join('controllers', 'uuv_tanh_2_15_2x32_broken.yml'),
            'mat_path': os.path.join('benchmark_contrrep', 'UUV', 'UUV_Phi_1_FFNN_32_32_YH_10_10_May_13.mat')},
}


# Repeated ISAR repairs of one benchmark on its 100-case test suite, without the verifier.
# Prepares the inputs once (robustness of the test suite under the original controller), then runs
# one repair per seed, each in a fresh process (for Simulink benchmarks, with its own MATLAB session).
# Results go to <results_dir>/<benchmark>/run_<seed>/; seeds whose repair already finished are skipped.
# A Simulink config listing several test suites (benchmark_opera/) gets one per seed: run <seed> repairs
# on test suite <seed>, whose inputs are prepared in <results_dir>/<benchmark>/testsuite_<seed>/.
if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("--benchmark", help="MC, UUV, or a Simulink benchmark folder under --benchmark_dir, e.g. AFC1_Phi1 or, with --benchmark_dir=benchmark_opera, ACC1")
    parser.add_argument("--benchmark_dir", help="folder of the Simulink benchmarks, benchmark_contrrep (default) or benchmark_opera", default='benchmark_contrrep')
    parser.add_argument("--runs", help="number of repeated runs, default 10, or one per test suite if the config lists several", default=None)
    parser.add_argument("--first_seed", help="seed of the first run; run i uses seed first_seed + i, default 1", default=1)
    parser.add_argument("--workers", help="Simulink only: number of MATLAB workers simulating in parallel", default=8)
    parser.add_argument("--breach_path", help="Simulink only: Breach installation", default=os.environ.get('ISAR_BREACH_PATH', os.path.join(os.path.dirname(ISAR_DIR), 'breach')))
    parser.add_argument("--results_dir", help="directory for all results", default=os.path.join(ISAR_DIR, 'results'))
    parser.add_argument("--eval_timeout", help="Simulink only: seconds to wait for one batch of simulations before restarting MATLAB, default 900", default=900)
    parser.add_argument("--std", help="standard deviation of the annealing perturbation; if given, runs go to <benchmark>/std_<std>/ instead of <benchmark>/ (default 0.1)", default=None)
    parser.add_argument("--prepare_only", help="1 to only prepare the inputs, e.g. once before submitting one job per seed", default=0)
    args = parser.parse_args()

    # Benchmarks outside benchmark_contrrep/ keep their results apart, e.g. benchmark_opera/ACC1 -> <results_dir>/opera/ACC1
    group = os.path.basename(os.path.normpath(args.benchmark_dir))
    group = '' if group == 'benchmark_contrrep' else group.replace('benchmark_', '', 1)
    out_dir = os.path.abspath(os.path.join(args.results_dir, group, args.benchmark))
    num_test_suites = 1
    if args.benchmark in PYTHON_BENCHMARKS:
        bm = PYTHON_BENCHMARKS[args.benchmark]
        kind = bm['benchmark']
        prepare_args = ['--mat_path=' + bm['mat_path'], '--network=' + bm['network']]
        repair_args = ['--network=' + bm['network']]
    else:
        configs = sorted(glob.glob(os.path.join(ISAR_DIR, args.benchmark_dir, args.benchmark, 'config_rfFR*')))
        assert len(configs) == 1, f'expected one config_rfFR* in {args.benchmark_dir}/{args.benchmark}, found {configs}'
        kind = 'simulink'
        from simulink_interface import parse_autopatch_config
        num_test_suites = len(parse_autopatch_config(configs[0])['testSuite'])
        if not os.path.isfile(os.path.join(args.breach_path, 'InitBreach.m')):
            sys.exit(f'Breach not found in {os.path.abspath(args.breach_path)} (no InitBreach.m); '
                     f'pass the Breach folder with --breach_path or set ISAR_BREACH_PATH')
        prepare_args = ['--simulink_config=' + configs[0], '--breach_path=' + os.path.abspath(args.breach_path),
                        '--workers=' + str(args.workers), '--eval_timeout=' + str(args.eval_timeout)]
        repair_args = prepare_args
    runs = int(args.runs) if args.runs is not None else (10 if num_test_suites == 1 else num_test_suites)
    seeds = [int(args.first_seed) + i for i in range(runs)]
    if num_test_suites > 1:
        assert max(seeds) <= num_test_suites, f'run <seed> uses test suite <seed>, but the config lists {num_test_suites} test suites'

    # The prepared inputs of the test suite a seed repairs on
    def prepared_dir(seed):
        return out_dir if num_test_suites == 1 else os.path.join(out_dir, f'testsuite_{seed}')

    def prepared_paths(seed):
        return (os.path.join(prepared_dir(seed), f'{kind}_verisig_result_skipped.csv'),
                os.path.join(prepared_dir(seed), f'{kind}_sampling_result_broken.csv'))

    def test_suite_args(seed):
        return [] if num_test_suites == 1 else ['--test_suite=' + str(seed)]

    # Run from the repository root: the UUV simulator reads uuv_model_oneHz.mat from the current folder
    for seed in sorted(set(seeds if num_test_suites > 1 else seeds[:1])):
        verisig_path, sampled_path = prepared_paths(seed)
        if not (os.path.isfile(verisig_path) and os.path.isfile(sampled_path)):
            suite = '' if num_test_suites == 1 else f' {seed}'
            print(f'Preparing {args.benchmark}: robustness of the test suite{suite} under the original controller', flush=True)
            subprocess.run([sys.executable, '-u', 'testsuite_to_isar.py', '--benchmark=' + kind, '--output_dir=' + prepared_dir(seed)]
                           + prepare_args + test_suite_args(seed), cwd=ISAR_DIR, check=True)
            shutil.rmtree(os.path.join(prepared_dir(seed), 'matlab_work'), ignore_errors=True)
    if int(args.prepare_only):
        sys.exit(0)

    for seed in seeds:
        verisig_path, sampled_path = prepared_paths(seed)
        # Runs with a non-default perturbation are kept apart from the runs with the default of ISAR
        runs_dir = out_dir if args.std is None else os.path.join(out_dir, f'std_{float(args.std):g}')
        run_dir = os.path.join(runs_dir, f'run_{seed}')
        log_path = os.path.join(run_dir, 'repair.log')
        if os.path.isfile(log_path) and 'Total time:' in open(log_path).read():
            print(f'{args.benchmark} seed {seed}: already finished, skipped', flush=True)
            continue
        # A run that did not finish is started over in an empty folder, so no checkpoint of the earlier attempt remains
        if os.path.isdir(run_dir):
            print(f'{args.benchmark} seed {seed}: removing the unfinished earlier attempt in {run_dir}', flush=True)
            shutil.rmtree(run_dir)
        os.makedirs(run_dir)
        print(f'{args.benchmark} seed {seed}: repairing, log in {log_path}', flush=True)
        with open(log_path, 'w') as log:
            result = subprocess.run([sys.executable, '-u', 'incremental_repair.py', '--benchmark=' + kind, '--seed=' + str(seed),
                                     '--verisig_result_path=' + verisig_path, '--sampled_result_path=' + sampled_path,
                                     '--output_path=' + run_dir] + repair_args + test_suite_args(seed)
                                    + ([] if args.std is None else ['--std=' + str(float(args.std))]),
                                    cwd=ISAR_DIR, stdout=log, stderr=subprocess.STDOUT)
        if result.returncode != 0:
            print(f'{args.benchmark} seed {seed}: failed with exit code {result.returncode}, see {log_path}', flush=True)
            sys.exit(result.returncode)
        # The model copies made by fastRob are only needed while the run is active
        shutil.rmtree(os.path.join(run_dir, 'matlab_work'), ignore_errors=True)
        print(f'{args.benchmark} seed {seed}: done', flush=True)
