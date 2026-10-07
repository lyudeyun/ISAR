import argparse
import glob
import os
import subprocess
import sys


ISAR_DIR = os.path.dirname(os.path.abspath(__file__))

# Benchmarks simulated in Python: the controller and the 100-case test suite of each
PYTHON_BENCHMARKS = {
    'MC': {'benchmark': 'mc',
           'network': os.path.join('controllers', 'mc_sig_2x16_broken_old.yml'),
           'mat_path': os.path.join('benchmark', 'MC', 'MC_Phi_1_FFNN_16_16_XV_10_10_20250609.mat')},
    'UUV': {'benchmark': 'uuv',
            'network': os.path.join('controllers', 'uuv_tanh_2_15_2x32_broken.yml'),
            'mat_path': os.path.join('benchmark', 'UUV', 'UUV_Phi_1_FFNN_32_32_YH_10_10_May_13.mat')},
}


# Repeated ISAR repairs of one benchmark on its 100-case test suite, without the verifier.
# Prepares the inputs once (robustness of the test suite under the original controller), then runs
# one repair per seed, each in a fresh process (for Simulink benchmarks, with its own MATLAB session).
# Results go to <results_dir>/<benchmark>/run_<seed>/; seeds whose repair already finished are skipped.
if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("--benchmark", help="MC, UUV, or a Simulink benchmark folder under benchmark/, e.g. AFC1_Phi1")
    parser.add_argument("--runs", help="number of repeated runs, default 10", default=10)
    parser.add_argument("--first_seed", help="seed of the first run; run i uses seed first_seed + i, default 1", default=1)
    parser.add_argument("--workers", help="Simulink only: number of MATLAB workers simulating in parallel", default=8)
    parser.add_argument("--breach_path", help="Simulink only: Breach installation", default=os.environ.get('ISAR_BREACH_PATH', os.path.join('..', 'breach')))
    parser.add_argument("--results_dir", help="directory for all results", default=os.path.join(ISAR_DIR, 'results'))
    args = parser.parse_args()

    out_dir = os.path.abspath(os.path.join(args.results_dir, args.benchmark))
    if args.benchmark in PYTHON_BENCHMARKS:
        bm = PYTHON_BENCHMARKS[args.benchmark]
        kind = bm['benchmark']
        prepare_args = ['--mat_path=' + bm['mat_path'], '--network=' + bm['network']]
        repair_args = ['--network=' + bm['network']]
    else:
        configs = sorted(glob.glob(os.path.join(ISAR_DIR, 'benchmark', args.benchmark, 'config_rfFR*')))
        assert len(configs) == 1, f'expected one config_rfFR* in benchmark/{args.benchmark}, found {configs}'
        kind = 'simulink'
        prepare_args = ['--simulink_config=' + configs[0], '--breach_path=' + os.path.abspath(args.breach_path),
                        '--workers=' + str(args.workers)]
        repair_args = prepare_args
    verisig_path = os.path.join(out_dir, f'{kind}_verisig_result_skipped.csv')
    sampled_path = os.path.join(out_dir, f'{kind}_sampling_result_broken.csv')

    # Run from the repository root: the UUV simulator reads uuv_model_oneHz.mat from the current folder
    if not (os.path.isfile(verisig_path) and os.path.isfile(sampled_path)):
        print(f'Preparing {args.benchmark}: robustness of the test suite under the original controller', flush=True)
        subprocess.run([sys.executable, '-u', 'testsuite_to_isar.py', '--benchmark=' + kind, '--output_dir=' + out_dir]
                       + prepare_args, cwd=ISAR_DIR, check=True)

    for i in range(int(args.runs)):
        seed = int(args.first_seed) + i
        run_dir = os.path.join(out_dir, f'run_{seed}')
        log_path = os.path.join(run_dir, 'repair.log')
        if os.path.isfile(log_path) and 'Total time:' in open(log_path).read():
            print(f'{args.benchmark} seed {seed}: already finished, skipped', flush=True)
            continue
        os.makedirs(run_dir, exist_ok=True)
        print(f'{args.benchmark} seed {seed}: repairing, log in {log_path}', flush=True)
        with open(log_path, 'w') as log:
            result = subprocess.run([sys.executable, '-u', 'incremental_repair.py', '--benchmark=' + kind, '--seed=' + str(seed),
                                     '--verisig_result_path=' + verisig_path, '--sampled_result_path=' + sampled_path,
                                     '--output_path=' + run_dir] + repair_args,
                                    cwd=ISAR_DIR, stdout=log, stderr=subprocess.STDOUT)
        if result.returncode != 0:
            print(f'{args.benchmark} seed {seed}: failed with exit code {result.returncode}, see {log_path}', flush=True)
            sys.exit(result.returncode)
        print(f'{args.benchmark} seed {seed}: done', flush=True)
