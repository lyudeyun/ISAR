import argparse
import os
from incremental_repair_utils import *
from simulink_interface import DEFAULT_BREACH_PATH


# Write a csv under a temporary name and rename it, so that a concurrent reader never sees a partial file
def write_csv_atomically(df, path):
    tmp_path = f'{path}.tmp{os.getpid()}'
    df.to_csv(tmp_path, index=False)
    os.replace(tmp_path, path)


# Simulink benchmark: the test suite named in the AutoPatch config, simulated with the original controller
def convert_simulink(args):
    from simulink_interface import SimulinkBenchmark
    bench = SimulinkBenchmark(os.path.dirname(args.simulink_config), args.simulink_config, args.breach_path,
                              os.path.join(args.output_dir, 'matlab_work'), workers=int(args.workers))
    try:
        # Robustness is recomputed with the same simulation pipeline as during repair
        case_ids = list(range(bench.num_cases))
        robustness = np.array(bench.robustness(bench.original_net(), case_ids))
    finally:
        bench.close()
    stored = bench.stored_rob
    print(f'Recomputed robustness vs. rob in the test suite: max |diff| = {np.max(np.abs(robustness - stored)):.2e}, '
          f'sign mismatches = {int(np.sum((robustness < 0) != (stored < 0)))}')

    df_sample = pd.DataFrame({'region': case_ids, 'case': case_ids, 'result': robustness})
    df_regions = pd.DataFrame({'case': case_ids})
    df_verisig = df_regions.copy()
    df_verisig['result'] = 'unknown'
    os.makedirs(args.output_dir, exist_ok=True)
    write_csv_atomically(df_regions, os.path.join(args.output_dir, 'simulink_initial_state_regions.csv'))
    write_csv_atomically(df_verisig, os.path.join(args.output_dir, 'simulink_verisig_result_skipped.csv'))
    write_csv_atomically(df_sample, os.path.join(args.output_dir, 'simulink_sampling_result_broken.csv'))
    print(f'{len(df_sample)} test cases, {(df_sample["result"] < 0).sum()} violating')


# Convert a MATLAB test suite of initial states into the input files of incremental_repair.py,
# for running ISAR without the verifier. Each test case becomes its own region, so every good state
# is protected during repair, and every bad state is repaired as a separate region.
if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("--benchmark", help="uuv, mc or simulink", default="uuv")
    parser.add_argument("--mat_path", help="uuv and mc: MATLAB file with a testSuite struct whose testCases have sysInputs and rob; "
                                           "sysInputs is [y, h] for uuv and [pos, vel] for mc")
    parser.add_argument("--network", help="uuv and mc: network yml file to be repaired", default=os.path.join('controllers', 'uuv_tanh_2_15_2x32_broken.yml'))
    parser.add_argument("--output_dir", help="directory to write the csv files")
    parser.add_argument("--simulink_config", help="simulink only: AutoPatch config file in the benchmark folder, e.g. benchmark/AFC1_Phi1/config_rfFR11")
    parser.add_argument("--breach_path", help="simulink only: Breach installation", default=DEFAULT_BREACH_PATH)
    parser.add_argument("--workers", help="simulink only: number of MATLAB workers simulating in parallel", default=1)
    args = parser.parse_args()

    if args.benchmark == 'simulink':
        convert_simulink(args)
        exit()

    test_cases = sio.loadmat(args.mat_path, squeeze_me=True, struct_as_record=False)['testSuite'].testCases

    with open(args.network, 'rb') as f:
        model_dict = yaml.safe_load(f)
    if args.benchmark == 'uuv':
        net = UUV_Control_NN()
        x1, x2 = 'y', 'h'
    elif args.benchmark == 'mc':
        net = MC_Control_NN()
        x1, x2 = 'pos', 'vel'
    else:
        raise NotImplementedError
    load_model_dict(model_dict, net)

    # Robustness is recomputed by simulation, so that it is consistent with the checks during repair
    rows = []
    max_diff = 0.0
    sign_mismatch = 0
    for idx, test_case in enumerate(test_cases):
        v1, v2 = float(test_case.sysInputs[0]), float(test_case.sysInputs[1])
        if args.benchmark == 'uuv':
            _, traj_pos_y = uuv_simulate(net=net, init_global_heading_deg=v2, init_pos_y=v1)
            robustness = uuv_robustness(traj_pos_y)
        else:
            traj_pos, _ = mc_simulate(v1, v2, net)
            robustness = mc_robustness(traj_pos)
        rows.append({'region': idx, x1: v1, x2: v2, 'result': robustness})
        max_diff = max(max_diff, abs(robustness - float(test_case.rob)))
        sign_mismatch += (robustness < 0) != (float(test_case.rob) < 0)

    df_sample = pd.DataFrame(rows, columns=['region', x1, x2, 'result'])
    df_regions = pd.DataFrame({f'{x1}_lo': df_sample[x1], f'{x1}_hi': df_sample[x1],
                               f'{x2}_lo': df_sample[x2], f'{x2}_hi': df_sample[x2]})
    df_verisig = df_regions.copy()
    df_verisig['result'] = 'unknown'

    os.makedirs(args.output_dir, exist_ok=True)
    write_csv_atomically(df_regions, os.path.join(args.output_dir, f'{args.benchmark}_initial_state_regions.csv'))
    write_csv_atomically(df_verisig, os.path.join(args.output_dir, f'{args.benchmark}_verisig_result_skipped.csv'))
    write_csv_atomically(df_sample, os.path.join(args.output_dir, f'{args.benchmark}_sampling_result_broken.csv'))

    print(f'{len(df_sample)} test cases, {(df_sample["result"] < 0).sum()} violating')
    print(f'Recomputed robustness vs. rob in the mat file: max |diff| = {max_diff:.2e}, sign mismatches = {sign_mismatch}')
