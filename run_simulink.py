import argparse
import glob
import os
import subprocess
import sys


ISAR_DIR = os.path.dirname(os.path.abspath(__file__))


# Repeated ISAR repairs of one Simulink benchmark, without the verifier.
# Prepares the inputs once (robustness of the test suite under the original controller), then runs
# one repair per seed, each in a fresh process with its own MATLAB session. Results go to
# <results_dir>/<benchmark>/run_<seed>/; seeds whose repair already finished are skipped.
if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("--benchmark", help="benchmark folder name under benchmark/, e.g. AFC1_Phi1")
    parser.add_argument("--runs", help="number of repeated runs, default 10", default=10)
    parser.add_argument("--first_seed", help="seed of the first run; run i uses seed first_seed + i, default 1", default=1)
    parser.add_argument("--workers", help="number of MATLAB workers simulating in parallel", default=8)
    parser.add_argument("--breach_path", help="Breach installation", default=os.environ.get('ISAR_BREACH_PATH', os.path.join('..', 'breach')))
    parser.add_argument("--results_dir", help="directory for all results", default='results')
    args = parser.parse_args()

    configs = sorted(glob.glob(os.path.join(ISAR_DIR, 'benchmark', args.benchmark, 'config_rfFR*')))
    assert len(configs) == 1, f'expected one config_rfFR* in benchmark/{args.benchmark}, found {configs}'
    config = configs[0]
    out_dir = os.path.join(args.results_dir, args.benchmark)
    verisig_path = os.path.join(out_dir, 'simulink_verisig_result_skipped.csv')
    sampled_path = os.path.join(out_dir, 'simulink_sampling_result_broken.csv')
    common = ['--simulink_config=' + config, '--breach_path=' + args.breach_path, '--workers=' + str(args.workers)]

    if not (os.path.isfile(verisig_path) and os.path.isfile(sampled_path)):
        print(f'Preparing {args.benchmark}: robustness of the test suite under the original controller', flush=True)
        subprocess.run([sys.executable, '-u', os.path.join(ISAR_DIR, 'testsuite_to_isar.py'), '--benchmark=simulink',
                        '--output_dir=' + out_dir] + common, check=True)

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
            result = subprocess.run([sys.executable, '-u', os.path.join(ISAR_DIR, 'incremental_repair.py'), '--benchmark=simulink',
                                     '--seed=' + str(seed), '--verisig_result_path=' + verisig_path,
                                     '--sampled_result_path=' + sampled_path, '--output_path=' + run_dir] + common,
                                    stdout=log, stderr=subprocess.STDOUT)
        if result.returncode != 0:
            print(f'{args.benchmark} seed {seed}: failed with exit code {result.returncode}, see {log_path}', flush=True)
            sys.exit(result.returncode)
        print(f'{args.benchmark} seed {seed}: done', flush=True)
