# Repeatability Package of Incremental Simulated Annealing Repair

### Step 0: Prepare required packages

This is the repeatability package for ICCPS 2024 Submission "Repairing Learning-Enabled Controllers While Preserving What Works".
To run our code, `Python >= 3.7.0` needs to be installed, and then the required packages shall be installed by
```
pip install -r requirements.txt
```

With recent package versions, the code fails: it uses `DataFrame.append`, which was removed in pandas 2.0, and loads whole PyTorch models with `torch.load`, which loads weights only by default since PyTorch 2.6.
Use a dedicated environment with older versions instead. The following environment was tested on macOS (Apple Silicon):

```
conda create -n isar python=3.10 -y
conda activate isar
pip install "numpy<2" "pandas<2" "torch<2.6" "scipy==1.13.1" pyyaml matplotlib
```

- `numpy<2` is required by pandas 1.5.
- `scipy==1.13.1`, since newer scipy wheels failed to load on recent macOS.
- `gym`, `stable_baselines3` and `casadi` in `requirements.txt` are only needed for the baselines in `mc_baselines/` and `uuv_baselines/`.

To check the environment, the following should print `1.5.3 2.5.1 True`:

```
python -c "import pandas as pd, torch; print(pd.__version__, torch.__version__, hasattr(pd.DataFrame, 'append'))"
```

The Simulink benchmarks in `benchmark_contrrep/` and `benchmark_opera/` are simulated in MATLAB through the MATLAB Engine for Python. Install it into the same environment from the local MATLAB installation (MATLAB R2026a on macOS here):

```
pip install /Applications/MATLAB_R2026a.app/extern/engines/python
```

The following should print the MATLAB version (starting MATLAB may take a while):

```
python -c "import matlab.engine; eng = matlab.engine.start_matlab(); print(eng.version()); eng.quit()"
```

The supported Python versions depend on the MATLAB release: R2026a supports Python 3.9 to 3.13, while R2021a supports only Python 3.7 and 3.8.
For MATLAB R2021a (e.g. on a Linux server), use Python 3.8 instead.
The engine is installed from the MATLAB installation folder, which is two levels above the `matlab` command (e.g. `/data1/deyun/MATLAB/R2021a` for `/data1/deyun/MATLAB/R2021a/bin/matlab`):

```
conda create -n isar python=3.8 -y
conda activate isar
conda env config vars set PYTHONNOUSERSITE=1
conda deactivate && conda activate isar
pip install "numpy<2" "pandas<2" "torch<2.5" scipy pyyaml matplotlib
pip install "setuptools<66"
MATLABROOT=$(dirname $(dirname $(readlink -f $(which matlab))))
cd $MATLABROOT/extern/engines/python
python setup.py build --build-base=$HOME/tmp/matlab_engine_build install
```

- `torch<2.5`, since PyTorch 2.5 dropped Python 3.8.
- `setuptools<66`, since the `setup.py` of R2021a gives its version as `R2021a`, which setuptools 66 and later reject (`InvalidVersion: 'R2021a'`). The deprecation warnings printed during the installation can be ignored.
- `--build-base` builds the engine in a folder of your own, in case the MATLAB installation is not writable.
- The engine is only needed for the Simulink benchmarks; MC and UUV run without MATLAB.
- `PYTHONNOUSERSITE=1` makes the environment ignore packages installed with `pip install --user` (in `~/.local`). Otherwise they take precedence over the versions above and can also break the engine installation (`packaging` from `~/.local` raising `InvalidVersion: 'R2021a'`). `python -c "import numpy, pandas, torch, packaging; print(numpy.__file__, pandas.__file__, torch.__file__, packaging.__file__)"` must show only paths inside the environment.

Check the environment with the same two commands as above. The first one prints other versions here (e.g. `1.5.3 2.4.1 True`), but must end with `True`.

### Step 1: Generate iniitial state space partitions

First, we partition the initial state regions by calling

```
python generate_partition.py --benchmark=<uuv|mc>
```

This will generate a partition of initial state space for UUV or MC. The partition is saved as a csv file, with each rectangular
region's lower bound and higher bound of every dimension.


### Step 2: Verify controller networks on initial state regions

We call Verisig, a sound but incomplete verifier, to verify whether a control policy network can satisfy the given STL task on each partitioned region.
The first step is to install Verisig. A copy of the Verisig source [https://github.com/verisig/verisig] is included in `verisig/`, which is also the default `--verisig_path`.
The following steps are from the Quick Start section of `verisig/UserManual.pdf`, tested by the Verisig authors on Ubuntu 16.04 with Java 1.8:

```
# 1. Install Flow* libraries
apt install libgmp3-dev libmpfr-dev libmpfr-doc libmpfr4 libmpfr4-dbg gsl-bin libgsl0-dev bison flex gnuplot-x11 libglpk-dev

# 2. Compile Flow*
cd verisig/flowstar; make; cd ..

# 3. Compile Verisig
cd verisig-src; ./gradlew installDist; cd ..

# 4. Run the Mountain Car example, which should end with a SAFE result
./verisig --flowstar-cmd=./flowstar/flowstar examples/mountain_car/MC.xml examples/mountain_car/sig16x16.yml
```

Notes:
- `verisig/flowstar/Makefile` also links against `yaml-cpp`, which is not listed in the manual (`apt install libyaml-cpp-dev`).
- The library paths used by `make` are set in `verisig/flowstar/makefile.local` (default `/usr/local/include` and `/usr/local/lib`). Change them if the libraries are installed elsewhere, e.g. `/opt/homebrew/include` and `/opt/homebrew/lib` on Apple Silicon macOS.

After Verisig is installed, please notice that we have prepared a `.cfg` and `.xml` file for both UUV and MC. These files encode the system dynamics.
We also have a `...multi.yml` for both. This file encodes the STL specification.

The initial broken controller networks are in `/controllers`. Verisig requires `.yml` files for neural network controllers.

Two versions of the initial MC controller are included. Both have a 2-16-16-1 architecture with Sigmoid, Sigmoid, and Tanh activations, but different weights and biases:

| Model file | Version |
| --- | --- |
| `controllers/mc_sig_2x16_broken_new.yml` | Updated MC controller from upstream commit `dd60cb1` (2025-07-10). This is the default MC model. |
| `controllers/mc_sig_2x16_broken_old.yml` | Original, lower-performing MC controller from upstream commit `fd67e94` (2024-01-29). |

Select the version explicitly with `--network`. Generate sampling and verification results separately for each model, using distinct output paths; do not reuse one model's results for the other. The UUV controller is unchanged.

With Verisig installed and all files prepared, we call the following code for parallel verification on initial state regions.

```
python verisig_call.py --benchmark=<uuv|mc> --network=<control network yaml file> --verisig_path=<directory where verisig is installed> --verisig_output_path=<directory to write verisig output txt files> --cpu_ratio=<percentage of cpus to be used, between 0 and 1> --initial_state_regions_path=<csv file of partitioned regions from previous step>
```

Warning: Running Verisig parallel verification may take a very long time (please expect more than 12 hours), depending on CPU utilization. Verification log of each initial state region will be recorded in a txt file in the specified output directory. This will take a large storage (please expect over 1GB).

After all regions are verified, we can parse the results by calling
```
python verisig_parse_results.py --benchmark=<uuv|mc> --network=<control network yaml file> --verisig_output_path=<directory of verisig output txt files> --initial_state_regions_csv=<csv file of partitioned regions from previous step>
```
The verification of each region will be run in a subprocess, output to a txt log file.
The verification results of all initial state regions will be parsed from Verisig logs and written to another csv file.
Execution time of verifying each region will be recorded at the end of every txt log file.

### Step 3: Repair controller networks

**Please notice that due to randomness (from random sampling of initial states per region and random perturbation in simulated annealing), the output may not necessarily be the same as in our paper.**

After verification on the broken controller is done, we repair the controller by Incremental Simulated Annealing Repair (ISAR). 
First, we uniformly sample initial states from each region.
```
python sample_states_in_regions.py --benchmark=<uuv|mc> --network=<control network yaml to be repaired> --initial_state_regions_path=<csv file of partitioned regions from previous step> --sampled_result_path=<ampling result csv to be written> --num_samples_per_region=<a positive int, by default 10>
```
This will sample a fixed number of initial states per region and obtain STL robustness of each sampled state. The result will be written in the specified sampled result path as a csv.

Next, with initial states sampled, we run the ISAR algorithm by calling
```
python incremental_repair.py --benchmark=<uuv|mc> --network=<control network yaml to be repaired> --verisig_result_path=<verisig result csv> --sampled_result_path=<sampling result csv> --output_path=<directory to all output files>
```
This is the main repair algorithm. At every iteration, the network will be checkpointed as both yaml and PyTorch files if the selected region is repaired and no good sampled states are broken.
The new STL robustness on all sampled states will also be checkpointed as csv files.
Please expect that this will take a long time, around 1-2 days. Notice that the system dynamics informaion is encoded in `uuv_model_oneHz.mat`.
Since the repaired network and the new STL robustness will be checkpointed after every iteration, we can always early stop and use one of the checkpoints as the output.
Execution time of repairing each region will be displayed in the standard output.


### Step 4: Verify the repaired controller
The repaired network can be verified again using Verisig. It is the same operation as step 2, except that the input network yaml file into `verisig_call.py` is replaced by the repaired file.
Notice that we don't necessarily need to use the very last repaired network - any checkpointed network during the repair procedure can also be verified again and see the outcome.


### Step 5: Visualize results
Once we have a verification result and a sampled result (both as csv files) for a controller network, we can visualize the outcome as in our paper.
By calling
```
python visualization.py --benchmark=<uuv|mc> --verisig_result_path=<verisig result csv> --sampled_result_path=<sampling result csv> 
```
It will end up with a plot like the follows.

![UUV result example](figures/uuv_broken_result.png)

Configurations of this plot, such as title, ticks and size can be modified in `visualization.py`.
This code will also output the number of the three types of regions,
and the mean and std of min STL robustness in red, non-red and overall regions (as in our Table 1 and 3).

### Note on the MC dynamics

The MC simulator used for sampling and repair (`mc_simulate` in `incremental_repair_utils.py`) updates the position with the velocity of the previous step (explicit Euler; clipping of position and velocity omitted below):

```
pos[t+1] = pos[t] + vel[t]
vel[t+1] = vel[t] + 0.0015 * u[t] - 0.0025 * cos(3 * pos[t])
```

This differs slightly from the standard Mountain Car (Sutton & Barto, OpenAI Gym), which updates the position with the new velocity, `pos[t+1] = pos[t] + vel[t+1]`.
The Verisig model `mc.xml` and the gym environment `mc_baselines/mc_gym.py` follow the standard form.
We keep the ISAR implementation unchanged, so that results remain comparable with the original ISAR implementation. When comparing other methods on MC, use the same dynamics as `mc_simulate`.

### Example: the full pipeline on UUV

The commands below run Steps 1-5 on the UUV benchmark with concrete file names, so they can be copied as is.
Run them from the repository root, after installing the packages (Step 0) and Verisig into `verisig/` (Step 2).

All generated files are kept in `results/uuv/`. Some scripts take the output path as an argument; the others always write to the repository root, so their files are moved into `results/uuv/` afterwards.
The long-running commands are wrapped in `/usr/bin/time -p python -u ... 2>&1 | tee results/uuv/logs/<name>.log`, which saves the full terminal output and appends the total wall-clock time in seconds as a `real` line at the end of the log.

```
results/uuv/
├── uuv_initial_state_regions.csv        # Step 1
├── verisig_output_broken/               # Step 2, one Verisig log per region
├── uuv_verisig_result_broken.csv        # Step 2
├── uuv_sampling_result_broken.csv       # Step 3
├── repair_output/                       # Step 3, repair checkpoints
├── verisig_output_repaired/             # Step 4
├── uuv_verisig_result_repaired.csv      # Step 4
├── uuv_*_execution_log.txt              # Steps 2 and 4, verification time per region
└── logs/                                # terminal output and total time of each command
```

#### 1. Partition the initial state space

```
mkdir -p results/uuv/logs
python generate_partition.py --benchmark=uuv
mv uuv_initial_state_regions.csv results/uuv/
```

#### 2. Verify the broken controller

Writes one txt log per region to `results/uuv/verisig_output_broken/`, and the verification time per region to `uuv_uuv_tanh_2_15_2x32_broken_execution_log.txt`.

```
/usr/bin/time -p python -u verisig_call.py --benchmark=uuv --network=controllers/uuv_tanh_2_15_2x32_broken.yml --verisig_path=verisig --verisig_output_path=results/uuv/verisig_output_broken --cpu_ratio=0.5 --initial_state_regions_path=results/uuv/uuv_initial_state_regions.csv 2>&1 | tee results/uuv/logs/uuv_verify_broken.log
mv uuv_*_execution_log.txt results/uuv/
```

Parse the logs. The result is always written to `uuv_verisig_result.csv` in the repository root, so move and rename it.

```
python verisig_parse_results.py --benchmark=uuv --network=controllers/uuv_tanh_2_15_2x32_broken.yml --verisig_output_path=results/uuv/verisig_output_broken --initial_state_regions_csv=results/uuv/uuv_initial_state_regions.csv
mv uuv_verisig_result.csv results/uuv/uuv_verisig_result_broken.csv
```

#### 3. Sample initial states and repair

Sample 10 initial states per region.

```
/usr/bin/time -p python -u sample_states_in_regions.py --benchmark=uuv --network=controllers/uuv_tanh_2_15_2x32_broken.yml --initial_state_regions_path=results/uuv/uuv_initial_state_regions.csv --sampled_result_path=results/uuv/uuv_sampling_result_broken.csv --num_samples_per_region=10 2>&1 | tee results/uuv/logs/uuv_sampling_broken.log
```

Run ISAR. Checkpoints `tanh_iter_<i>_region_<r>.yml/.pt` and `sample_checkpoint_iter_<i>_region_<r>.csv` are written to `results/uuv/repair_output/`. Per-iteration times are printed as `Iteration time` and kept in the log.

```
/usr/bin/time -p python -u incremental_repair.py --benchmark=uuv --network=controllers/uuv_tanh_2_15_2x32_broken.yml --verisig_result_path=results/uuv/uuv_verisig_result_broken.csv --sampled_result_path=results/uuv/uuv_sampling_result_broken.csv --output_path=results/uuv/repair_output 2>&1 | tee results/uuv/logs/uuv_repair.log
```

#### 4. Verify the repaired controller

Pick the latest checkpoint and its sampled result. To use an earlier checkpoint, set `REPAIRED` to its `.yml` path instead.

```
REPAIRED=$(ls -t results/uuv/repair_output/tanh_iter_*.yml | head -1)
REPAIRED_SAMPLES=$(echo "$REPAIRED" | sed 's/tanh_iter_/sample_checkpoint_iter_/; s/\.yml$/.csv/')
```

Verify it, then parse, move and rename the result. Run this in the same shell as above, so that `$REPAIRED` is still set.

```
/usr/bin/time -p python -u verisig_call.py --benchmark=uuv --network="$REPAIRED" --verisig_path=verisig --verisig_output_path=results/uuv/verisig_output_repaired --cpu_ratio=0.5 --initial_state_regions_path=results/uuv/uuv_initial_state_regions.csv 2>&1 | tee results/uuv/logs/uuv_verify_repaired.log
mv uuv_*_execution_log.txt results/uuv/
python verisig_parse_results.py --benchmark=uuv --network="$REPAIRED" --verisig_output_path=results/uuv/verisig_output_repaired --initial_state_regions_csv=results/uuv/uuv_initial_state_regions.csv
mv uuv_verisig_result.csv results/uuv/uuv_verisig_result_repaired.csv
```

#### 5. Visualize the results

Broken controller:

```
python visualization.py --benchmark=uuv --verisig_result_path=results/uuv/uuv_verisig_result_broken.csv --sampled_result_path=results/uuv/uuv_sampling_result_broken.csv
```

Repaired controller (in the same shell as Step 4, so that `$REPAIRED_SAMPLES` is still set):

```
python visualization.py --benchmark=uuv --verisig_result_path=results/uuv/uuv_verisig_result_repaired.csv --sampled_result_path="$REPAIRED_SAMPLES"
```

### Example: the full pipeline on MC

The same pipeline for MC with the new controller `controllers/mc_sig_2x16_broken_new.yml`. All generated files are kept in `results/mc_new/`.
The two MC controllers must be kept in separate folders, since most generated file names do not include the controller version.
To run the old controller, replace every `new` with `old` in the commands below (`mc_sig_2x16_broken_old.yml` and `results/mc_old/`).

#### 1. Partition the initial state space

```
mkdir -p results/mc_new/logs
python generate_partition.py --benchmark=mc
mv mc_initial_state_regions.csv results/mc_new/
```

#### 2. Verify the broken controller

Writes one txt log per region to `results/mc_new/verisig_output_broken/`, and the verification time per region to `mc_mc_sig_2x16_broken_new_execution_log.txt`.

```
/usr/bin/time -p python -u verisig_call.py --benchmark=mc --network=controllers/mc_sig_2x16_broken_new.yml --verisig_path=verisig --verisig_output_path=results/mc_new/verisig_output_broken --cpu_ratio=0.5 --initial_state_regions_path=results/mc_new/mc_initial_state_regions.csv 2>&1 | tee results/mc_new/logs/mc_verify_broken.log
mv mc_*_execution_log.txt results/mc_new/
```

Parse the logs. The result is always written to `mc_verisig_result.csv` in the repository root, so move and rename it.

```
python verisig_parse_results.py --benchmark=mc --network=controllers/mc_sig_2x16_broken_new.yml --verisig_output_path=results/mc_new/verisig_output_broken --initial_state_regions_csv=results/mc_new/mc_initial_state_regions.csv
mv mc_verisig_result.csv results/mc_new/mc_verisig_result_broken.csv
```

#### 3. Sample initial states and repair

Sample 10 initial states per region.

```
/usr/bin/time -p python -u sample_states_in_regions.py --benchmark=mc --network=controllers/mc_sig_2x16_broken_new.yml --initial_state_regions_path=results/mc_new/mc_initial_state_regions.csv --sampled_result_path=results/mc_new/mc_sampling_result_broken.csv --num_samples_per_region=10 2>&1 | tee results/mc_new/logs/mc_sampling_broken.log
```

Run ISAR. Checkpoints `tanh_iter_<i>_region_<r>.yml/.pt` and `sample_checkpoint_iter_<i>_region_<r>.csv` are written to `results/mc_new/repair_output/`.

```
/usr/bin/time -p python -u incremental_repair.py --benchmark=mc --network=controllers/mc_sig_2x16_broken_new.yml --verisig_result_path=results/mc_new/mc_verisig_result_broken.csv --sampled_result_path=results/mc_new/mc_sampling_result_broken.csv --output_path=results/mc_new/repair_output 2>&1 | tee results/mc_new/logs/mc_repair.log
```

#### 4. Verify the repaired controller

Pick the latest checkpoint and its sampled result. To use an earlier checkpoint, set `REPAIRED` to its `.yml` path instead.

```
REPAIRED=$(ls -t results/mc_new/repair_output/tanh_iter_*.yml | head -1)
REPAIRED_SAMPLES=$(echo "$REPAIRED" | sed 's/tanh_iter_/sample_checkpoint_iter_/; s/\.yml$/.csv/')
```

Verify it, then parse, move and rename the result. Run this in the same shell as above, so that `$REPAIRED` is still set.

```
/usr/bin/time -p python -u verisig_call.py --benchmark=mc --network="$REPAIRED" --verisig_path=verisig --verisig_output_path=results/mc_new/verisig_output_repaired --cpu_ratio=0.5 --initial_state_regions_path=results/mc_new/mc_initial_state_regions.csv 2>&1 | tee results/mc_new/logs/mc_verify_repaired.log
mv mc_*_execution_log.txt results/mc_new/
python verisig_parse_results.py --benchmark=mc --network="$REPAIRED" --verisig_output_path=results/mc_new/verisig_output_repaired --initial_state_regions_csv=results/mc_new/mc_initial_state_regions.csv
mv mc_verisig_result.csv results/mc_new/mc_verisig_result_repaired.csv
```

#### 5. Visualize the results

Broken controller:

```
python visualization.py --benchmark=mc --verisig_result_path=results/mc_new/mc_verisig_result_broken.csv --sampled_result_path=results/mc_new/mc_sampling_result_broken.csv
```

Repaired controller (in the same shell as Step 4, so that `$REPAIRED_SAMPLES` is still set):

```
python visualization.py --benchmark=mc --verisig_result_path=results/mc_new/mc_verisig_result_repaired.csv --sampled_result_path="$REPAIRED_SAMPLES"
```

### Example: running without the verifier

ISAR can also be run without Verisig. During repair, regions are colored by the sampled states only: a region is red (to be repaired) if any of its sampled states violates the STL specification, and yellow (to be preserved) otherwise.
This does not change the repair procedure itself, since green (verified) and yellow regions are treated the same way during repair. Verisig and Flow* are not needed, so Step 2 and Step 4 are skipped.

This is done by giving `incremental_repair.py` a placeholder verification result in which every region is marked `unknown`.
Use exactly `unknown`: the value `unknown?` is treated as verified (green), in which case no region is repaired.

Since the repair is a search, its cost is measured by the number of simulations rather than by wall-clock time. `incremental_repair.py` prints the number of simulations and saves it to `simulation_count.json` in the output directory after every iteration, so the counts are kept if the repair is stopped early. The counts are split into `annealing` (simulated annealing), `state_check` (checking bad and good states after annealing), `sample_check` (re-evaluating all sampled states after an accepted repair) and `total`, together with the number of completed `iterations`. The terminal output of each command is saved to `logs/`.

#### UUV

All generated files are kept in `results/uuv_no_verifier/`.

1. Partition the initial state space.

```
mkdir -p results/uuv_no_verifier/logs
python generate_partition.py --benchmark=uuv
mv uuv_initial_state_regions.csv results/uuv_no_verifier/
```

2. Mark every region as unverified.

```
python -c "import pandas as pd; df = pd.read_csv('results/uuv_no_verifier/uuv_initial_state_regions.csv'); df['result'] = 'unknown'; df.to_csv('results/uuv_no_verifier/uuv_verisig_result_skipped.csv', index=False)"
```

3. Sample 10 initial states per region. This runs one simulation per sampled state (10 x 2000 for UUV, 10 x 900 for MC), which is not included in the repair count.

```
python -u sample_states_in_regions.py --benchmark=uuv --network=controllers/uuv_tanh_2_15_2x32_broken.yml --initial_state_regions_path=results/uuv_no_verifier/uuv_initial_state_regions.csv --sampled_result_path=results/uuv_no_verifier/uuv_sampling_result_broken.csv --num_samples_per_region=10 2>&1 | tee results/uuv_no_verifier/logs/uuv_sampling_broken.log
```

4. Repair. Checkpoints and `simulation_count.json` are written to `results/uuv_no_verifier/repair_output/`.

```
python -u incremental_repair.py --benchmark=uuv --network=controllers/uuv_tanh_2_15_2x32_broken.yml --verisig_result_path=results/uuv_no_verifier/uuv_verisig_result_skipped.csv --sampled_result_path=results/uuv_no_verifier/uuv_sampling_result_broken.csv --output_path=results/uuv_no_verifier/repair_output 2>&1 | tee results/uuv_no_verifier/logs/uuv_repair.log
```

5. Visualize the broken controller and the latest repaired checkpoint. Regions are shown as yellow or red only.

```
python visualization.py --benchmark=uuv --verisig_result_path=results/uuv_no_verifier/uuv_verisig_result_skipped.csv --sampled_result_path=results/uuv_no_verifier/uuv_sampling_result_broken.csv
REPAIRED_SAMPLES=$(ls -t results/uuv_no_verifier/repair_output/sample_checkpoint_iter_*.csv | head -1)
python visualization.py --benchmark=uuv --verisig_result_path=results/uuv_no_verifier/uuv_verisig_result_skipped.csv --sampled_result_path="$REPAIRED_SAMPLES"
```

#### MC

The same steps for the new MC controller, with all generated files in `results/mc_new_no_verifier/`. For the old controller, replace every `new` with `old`.

1. Partition the initial state space.

```
mkdir -p results/mc_new_no_verifier/logs
python generate_partition.py --benchmark=mc
mv mc_initial_state_regions.csv results/mc_new_no_verifier/
```

2. Mark every region as unverified.

```
python -c "import pandas as pd; df = pd.read_csv('results/mc_new_no_verifier/mc_initial_state_regions.csv'); df['result'] = 'unknown'; df.to_csv('results/mc_new_no_verifier/mc_verisig_result_skipped.csv', index=False)"
```

3. Sample 10 initial states per region. This runs one simulation per sampled state (10 x 2000 for UUV, 10 x 900 for MC), which is not included in the repair count.

```
python -u sample_states_in_regions.py --benchmark=mc --network=controllers/mc_sig_2x16_broken_new.yml --initial_state_regions_path=results/mc_new_no_verifier/mc_initial_state_regions.csv --sampled_result_path=results/mc_new_no_verifier/mc_sampling_result_broken.csv --num_samples_per_region=10 2>&1 | tee results/mc_new_no_verifier/logs/mc_sampling_broken.log
```

4. Repair. Checkpoints and `simulation_count.json` are written to `results/mc_new_no_verifier/repair_output/`.

```
python -u incremental_repair.py --benchmark=mc --network=controllers/mc_sig_2x16_broken_new.yml --verisig_result_path=results/mc_new_no_verifier/mc_verisig_result_skipped.csv --sampled_result_path=results/mc_new_no_verifier/mc_sampling_result_broken.csv --output_path=results/mc_new_no_verifier/repair_output 2>&1 | tee results/mc_new_no_verifier/logs/mc_repair.log
```

5. Visualize the broken controller and the latest repaired checkpoint.

```
python visualization.py --benchmark=mc --verisig_result_path=results/mc_new_no_verifier/mc_verisig_result_skipped.csv --sampled_result_path=results/mc_new_no_verifier/mc_sampling_result_broken.csv
REPAIRED_SAMPLES=$(ls -t results/mc_new_no_verifier/repair_output/sample_checkpoint_iter_*.csv | head -1)
python visualization.py --benchmark=mc --verisig_result_path=results/mc_new_no_verifier/mc_verisig_result_skipped.csv --sampled_result_path="$REPAIRED_SAMPLES"
```

### Repairing the benchmarks on their test suites

Ten benchmarks can be repaired on a given test suite of 100 cases, without the verifier: MC and UUV, simulated in Python, and eight Simulink benchmarks in `benchmark_contrrep/`. Six more Simulink benchmarks, from OPERA, are in `benchmark_opera/` (see below).

| Benchmark | Simulator | Specification | Controller |
| --- | --- | --- | --- |
| `MC_old` | Python | MC | `controllers/mc_sig_2x16_broken_old.yml` |
| `UUV` | Python | UUV | `controllers/uuv_tanh_2_15_2x32_broken.yml` |
| `ACC1_Phi1` | Simulink | ACC4 | `ACC4_FFNN_trainbfg_15_15_15_ep_100_Nor_20250527` |
| `ACC1_Phi2` | Simulink | ACC5v | `ACC5v_FFNN_traingdx_15_15_15_ep_50_Nor_20250702` |
| `ACC2_Phi1` | Simulink | ACC4 | `ACC4_FFNN_trainoss_30_30_30_ep_100_Nor_20250528` |
| `ACC2_Phi2` | Simulink | ACC5v | `ACC5v_FFNN_traincgf_30_30_30_ep_50_Nor_20250704` |
| `AFC1_Phi1` | Simulink | AFC1 | `AFC1_FFNN_trainscg_15_15_15_ep_1000_Nor_20250518` |
| `AFC1_Phi2` | Simulink | AFC2 | `AFC2_FFNN_trainscg_15_15_15_ep_1000_Nor_20250717` |
| `AFC2_Phi1` | Simulink | AFC1 | `AFC1_FFNN_trainrp_15_15_15_15_ep_1000_Nor_20250519` |
| `AFC2_Phi2` | Simulink | AFC2 | `AFC2_FFNN_trainscg_15_15_15_15_ep_500_Nor_20250717` |

Each test case becomes its own region, so every satisfied test case is protected during repair and every violated one is repaired as a separate region.
Before repair, the robustness of the test suite under the original controller is recomputed by simulation, rather than taken from the test suite, so that it is consistent with the simulations during repair.
The MC test suite was generated with the old MC controller.

The simulated annealing is random, so the repair is repeated with different seeds.
`run_benchmark.py` prepares the inputs of a benchmark once, then runs one repair per seed, each in a new process:

```
python run_benchmark.py --benchmark=MC_old --runs=10
python run_benchmark.py --benchmark=UUV --runs=10
python run_benchmark.py --benchmark=AFC1_Phi1 --runs=10 --workers=8 --breach_path=/path/to/breach
```

Results are written to `results/contrrep/<benchmark>/`, with the file prefix `mc`, `uuv` or `simulink`:

```
results/contrrep/AFC1_Phi1/
├── simulink_sampling_result_broken.csv     # robustness of the test suite under the original controller
├── simulink_verisig_result_skipped.csv     # every test case marked unverified
├── simulink_initial_state_regions.csv
├── run_1/                                  # seed 1
│   ├── repair.log
│   ├── simulation_count.json
│   ├── tanh_iter_<i>_region_<r>.pt/.yml    # checkpoint after each accepted repair
│   └── sample_checkpoint_iter_<i>_region_<r>.csv
├── ...
└── run_10/                                 # seed 10
```

- Each seed writes only to its own `run_<seed>/` folder, so repeated runs never overwrite each other.
- Seeds whose repair already finished are skipped, so an interrupted call can simply be repeated. A seed that did not finish is started over in an empty `run_<seed>/` folder, so no checkpoint of the earlier attempt remains.
- The final repaired controller of a run is its last checkpoint, and its robustness on the test suite is in the matching `sample_checkpoint_*.csv`. If no region is repaired, there is no checkpoint and the result is the original controller.
- `simulation_count.json` counts the simulations of the repair, one per simulated test case. The simulations that prepare the inputs are not included.
- The repair stops once every violated region has been tried once; it has no simulation budget.
- The annealing perturbs every weight and bias with Gaussian noise of standard deviation 0.1, the default of ISAR. `--std` changes it, e.g. `--std=0.001`; such runs are written to `results/contrrep/<benchmark>/std_<std>/run_<seed>/`, apart from the runs with the default.

**Evaluation on the validation suite.** Each benchmark also has a validation suite of 10000 cases (`*_testCaseNum_10000_val.mat`), the held-out suite AutoPatch's `validatePatch` scores repairs on; for the Simulink benchmarks it is the `testSet` entry of the config. After the repairs, `evaluate_testset.py` simulates the final controller of each finished run on it, with the metrics described for the OPERA test set below (a case is satisfied if its robustness is > 0, as in `validatePatch`):

```
python evaluate_testset.py --benchmark=MC_old
python evaluate_testset.py --benchmark=UUV
python evaluate_testset.py --benchmark_dir=benchmark_contrrep --benchmark=AFC1_Phi1 --workers=8 --breach_path=/path/to/breach
```

The results sit next to those on the 100-case test suite, under names of their own:

```
results/contrrep/AFC1_Phi1/
├── simulink_sampling_result_broken.csv     # test suite (100): the original controller
├── testset/
│   ├── original.csv                        # validation suite (10000): the original controller
│   └── summary.csv                         # validation suite: all runs
├── run_1/
│   ├── sample_checkpoint_iter_<i>_region_<r>.csv   # test suite: after each accepted repair; the last is the result
│   ├── tanh_iter_<i>_region_<r>.pt/.yml            # the controller after each accepted repair; the last is the result
│   ├── testset_result.json                         # validation suite: SR before and after, cases repaired and broken
│   └── testset_robustness.csv                      # validation suite: robustness of every case before and after
└── ...
```

To run the seeds in parallel, e.g. one job per seed on a cluster, prepare the inputs once and then start one call per seed:

```
python run_benchmark.py --benchmark=AFC1_Phi1 --prepare_only=1 --workers=8 --breach_path=/path/to/breach
python run_benchmark.py --benchmark=AFC1_Phi1 --first_seed=1 --runs=1 --workers=8 --breach_path=/path/to/breach   # job 1
python run_benchmark.py --benchmark=AFC1_Phi1 --first_seed=2 --runs=1 --workers=8 --breach_path=/path/to/breach   # job 2
...
```

Never start two calls for the same seed at the same time: a call removes the folder of an unfinished seed before running it.
For Simulink benchmarks, give each concurrently running job its own copy of Breach and its own `MATLAB_JOB_STORAGE_LOCATION` (see the notes below).

The two steps that `run_benchmark.py` performs can also be called directly, e.g. for MC and AFC1_Phi1:

```
python testsuite_to_isar.py --benchmark=mc --mat_path=benchmark_contrrep/MC_old/MC_Phi_1_FFNN_16_16_XV_10_10_20250609.mat --network=controllers/mc_sig_2x16_broken_old.yml --output_dir=results/contrrep/MC_old
python -u incremental_repair.py --benchmark=mc --network=controllers/mc_sig_2x16_broken_old.yml --seed=1 --verisig_result_path=results/contrrep/MC_old/mc_verisig_result_skipped.csv --sampled_result_path=results/contrrep/MC_old/mc_sampling_result_broken.csv --output_path=results/contrrep/MC_old/run_1

python testsuite_to_isar.py --benchmark=simulink --simulink_config=benchmark_contrrep/AFC1_Phi1/config_rfFR11 --breach_path=/path/to/breach --workers=8 --output_dir=results/contrrep/AFC1_Phi1
python -u incremental_repair.py --benchmark=simulink --simulink_config=benchmark_contrrep/AFC1_Phi1/config_rfFR11 --breach_path=/path/to/breach --workers=8 --seed=1 --verisig_result_path=results/contrrep/AFC1_Phi1/simulink_verisig_result_skipped.csv --sampled_result_path=results/contrrep/AFC1_Phi1/simulink_sampling_result_broken.csv --output_path=results/contrrep/AFC1_Phi1/run_1
```

`testsuite_to_isar.py` also accepts other MATLAB test suites of UUV or MC initial states: a `testSuite` struct whose `testCases` have `sysInputs` ([y, h] for UUV, [pos, vel] for MC) and `rob`.

#### Simulink benchmarks

The eight Simulink benchmarks in `benchmark_contrrep/` were copied from the git-tracked files of the AutoPatch repository (commit `1edcc84`, `FLRepBenchGen/`).
Each benchmark folder contains the Simulink model, the controller network (`*_config_*.mat`), test suites of 100 cases (`*_testCaseNum_100.mat`, used for repair) and of 10000 cases (`*_testCaseNum_10000_val.mat`, for validation), the AutoPatch configs (`config_rfFR*`, `config_rfValEval*`), and the normalization parameters of the controller (`norm_params.mat`).
The specifications and simulation parameters are in the `config_rfFR*` file of each benchmark.
`benchmark_contrrep/MC_old/` and `benchmark_contrrep/UUV/` hold the test suites of MC (with the old controller) and UUV, with their 10000-case validation suites.

#### OPERA benchmarks

`benchmark_opera/` holds the six Simulink benchmarks of the OPERA repository, with the test suites its SpectAcle+ContrRep baseline repairs on (`baseline/spectacle+contrrep`, commit `6f4688767`), and MC with the new controller (`MC_new`, from `benchmark/mc/`, commit `a317187fb`; the same files are in `benchmark/mc/MC_Phi_1_FFNN_16_16_XV_100_100_Feb_11_2026/dataset/` since commit `c0bf9ba9e`), simulated in Python like `MC_old`.

| Benchmark | Specification | Model | Normalization |
| --- | --- | --- | --- |
| `ACC1` | ACC1 | `ACC_FFNN_trainlm_10_10_10_Nor_Sat_Feb_15` | yes |
| `ACC2` | ACC2 | `ACC_FFNN_trainbfg_30_30_30_Nor_Sat_Jan_24` | yes |
| `AFC1` | AFC1 | `AFC_FFNN_trainlm_15_15_15_NoSat_Jan_23` | no |
| `AFC2` | AFC2 | `AFC_FFNN_trainlm_30_30_30_Nor_Jan_6` | yes |
| `WT1` | WT1 | `WT_FFNN_trainbfg_5_5_5_Dec_22` | no |
| `WT2` | WT2 | `WT_FFNN_trainbfg_15_15_15_Dec_22` | no |
| `MC_new` | MC1 | `mc_sig_2x16_broken.yml` (= `controllers/mc_sig_2x16_broken_new.yml`) | – |

- The dataset to be repaired has 10000 traces, too many to simulate for every candidate of the annealing. Each benchmark therefore has five test suites of 100 cases (`*_TS_100_seed_<k>.mat`, k = 1..5), sampled stratified from that dataset, i.e. with its share of violating traces; the config (`config_rfFR_<benchmark>`) lists them in order. They are the first five of the ten test suites of OPERA's SpectAcle+ContrRep baseline.
- The repair is repeated with seeds 1 to 10 on each test suite, 50 runs per benchmark: the test suites capture the variation from sampling, the seeds that from the annealing.
- `MC_new` has the same layout without a model folder: the controller, the five test suites (`*_Tr_TS_100_seed_<k>.mat`) and the test set (`*_Te.mat`, 1000 cases) sit next to its config, whose `benchType nonSimulink` makes the scripts simulate it in Python. Its test suites have 1 violating case each, its test set 14.
- `norm_params.mat` holds the normalization of the network inputs and output, taken from the config's `dataset_run` as OPERA does (`ps_x`, `ps_y`); it is empty with `is_nor = 0` for the benchmarks without normalization.

```
python run_benchmark.py --benchmark_dir=benchmark_opera --benchmark=ACC1 --workers=8 --breach_path=/path/to/breach
```

`--test_suites` restricts the call to some test suites, e.g. `--test_suites=2` or `--test_suites=1-3`, and `--first_seed` and `--runs` select the seeds as above, so one job can run e.g. test suite 2 with seed 7:

```
python run_benchmark.py --benchmark_dir=benchmark_opera --benchmark=ACC1 --prepare_only=1 --workers=8 --breach_path=/path/to/breach
python run_benchmark.py --benchmark_dir=benchmark_opera --benchmark=ACC1 --test_suites=2 --first_seed=7 --runs=1 --workers=8 --breach_path=/path/to/breach
```

Results are written to `results/opera/<benchmark>/testsuite_<k>/`: the prepared inputs of test suite `<k>`, and its runs in `run_<seed>/` as above.

**Evaluation on the test set.** Each benchmark also has OPERA's test set of 1000 cases (`*_Te_TS_1000.mat`, the `testSet` entry of the config), converted from the `_Te.mat` file that OPERA's `rep_eval` evaluates repaired controllers on (`te_in_cell`, `te_rob_set`). After the repairs, `evaluate_testset.py` simulates the final controller of each finished run on it:

```
python evaluate_testset.py --benchmark=ACC1 --workers=8 --breach_path=/path/to/breach
```

- The final controller of a run is its last checkpoint, or the original controller if no region was repaired.
- The metrics are those of OPERA's `rep_eval`: a test case is satisfied if its robustness is > 0. `SR_before` and `SR_after` are the percentages of satisfied test cases under the original and the repaired controller, `neg_to_pos` the violated test cases that the repair satisfies, and `pos_to_neg` the satisfied ones it breaks. The original controller is simulated, like during repair, rather than taken from the test set.
- The original controller is evaluated once, in `results/opera/<benchmark>/testset/original.csv`. Each run gets `testset_robustness.csv` (robustness of every test case before and after repair) and `testset_result.json` in its `run_<seed>/` folder, and all runs are listed in `results/opera/<benchmark>/testset/summary.csv`.
- Runs that are already evaluated are skipped, and runs whose repair has not finished are left out, so the script can be run again while repairs are still going on. `--test_suites`, `--first_seed`, `--runs` and `--std` select the runs as for `run_benchmark.py`.
- The test set adds 1000 simulations per run, on top of 1000 for the original controller. Like a repair, the evaluation starts its own MATLAB session; run it with its own Breach copy if repairs are running at the same time.

**Performance metrics on the test set.** For the benchmarks of `benchmark_opera/`, `evaluate_testset.py` also computes system-specific performance metrics of each final controller, to assess how repair affects behaviour beyond satisfaction of the specification. The names and definitions below follow OPERA's `docs/metrics.md` and its RQ4 evaluation, including the constants and aggregation rules. The output field maps each metric to the raw value saved in `testset_metrics.json` and `testset/metrics_summary.csv`.

↑ means higher is better; ↓ means lower is better.

| Benchmark | Metric | Meaning | Output field |
| --- | --- | --- | --- |
| `ACC1`, `ACC2` | Space ↑ | Distance traveled by the ego car at the end of the run. | `te_mean_x_ego` |
| `ACC1`, `ACC2` | Speed ↓ | Mean absolute deviation from the set speed of 30 while the safety margin is positive. | `te_mean_delta_v` |
| `ACC1`, `ACC2` | Steadiness ↑ | Fraction of samples with a safety margin greater than 2 m. | `te_mean_steady` |
| `ACC1`, `ACC2` | Resilience ↑ | Fraction of unsteady intervals from which the car recovers within 1 s. | `te_mean_resilience` |
| `ACC1`, `ACC2` | Comfort ↑ | Fraction of steps with an absolute acceleration change below 0.1. | `te_mean_comfortable` |
| `AFC1`, `AFC2` | Steadiness ↑ | Fraction of samples with a relative air-to-fuel ratio deviation of at most 10%. | `te_mean_steady` |
| `AFC1`, `AFC2` | Resilience ↑ | Fraction of unsteady intervals from which the system recovers within 1 s. | `te_mean_recovery` |
| `AFC1`, `AFC2` | MAE ↓ | Mean relative air-to-fuel ratio deviation. | `te_mean_MAE` |
| `AFC1`, `AFC2` | MAXERR ↓ | Maximum relative air-to-fuel ratio deviation per run. | `te_mean_MAXERR` |
| `WT1`, `WT2` | Steadiness ↑ | Fraction of checked samples with an absolute water-level error below 0.5. | `te_mean_steady` |
| `WT1`, `WT2` | MAE ↓ | Mean absolute water-level error over the checked samples. | `te_mean_MAE` |
| `WT1`, `WT2` | MAXERR ↓ | Maximum absolute water-level error over the checked samples per run. | `te_mean_MAXERR` |
| `MC_new` | Reach ↓ | Steps to first reach position 0.45; 110 if the goal is never reached. | `te_mean_reach` |
| `MC_new` | Energy ↓ | Mean squared control signal per step up to the goal. | `te_mean_energy` |
| `MC_new` | Smoothness ↓ | Mean absolute change in the control signal per step up to the goal. | `te_mean_smooth` |

ACC, AFC and WT are sampled every 0.1 s. For ACC, the safety margin is `d_rel - 1.4 * v_ego - 10`, and samples with a margin below 2 m are unsteady. For AFC, the relative deviation is `abs(1 - AF / 14.7)`, and samples with a deviation above 0.10 are unsteady. An unsteady interval is a maximal sequence of consecutive unsteady samples; recovery means that the interval contains fewer than 10 samples. WT uses only the 30 samples at times 4.0–4.9, 9.0–9.9 and 14.0–14.9 s. MC uses the full 110 steps if the goal is never reached; Smoothness is 0 if it is reached at the first step.

Metrics are computed per test case and averaged over the test set, except for ACC Speed and ACC/AFC Resilience. Speed divides the total absolute speed deviation at safe samples by the total number of those samples; Resilience divides the total number of recovered intervals by the total number of unsteady intervals. These ratios are `NaN` when their denominator is zero. In particular, a controller with no unsteady intervals has undefined Resilience. AFC's Resilience keeps the output field name `te_mean_recovery` used by OPERA.

The implementations in OPERA are `test/rep_eval_acc.py`, `test/rep_eval_afc.py`, `test/rep_eval_wt.py` and `src/eval/MC_rep_eval.m` for ACC, AFC, WT and MC, respectively.

- Simulink benchmarks: the per-test-case quantities are computed by `simulink/isarOperaMetrics.m`, a copy of the MATLAB code that `rep_eval_*.py` generates, from the trajectory fastRob returns; the signals they need besides those of the specification are recorded through `simConf.extraSigs` (see `simulink/fastrob/README.md`). `evaluate_testset.py` averages them as `rep_eval_*.py` does, e.g. `te_mean_resilience = sum(recovery) / sum(unsteady)`.
- MC: simulated as `simMatlabMC.m` (which `MC_rep_eval.m` uses), in double precision from the controller's yml file, and scored with `mcMetrics` of `MC_rep_eval.m`. `mc_simulate`, used during repair, evaluates the network in single precision, which moves the metrics by about 1e-7.
- The metrics are computed in a simulation session of their own: the robustness and the files above are as without them.
- Outputs: in each run folder `testset_metrics.json` (the metrics of its final controller, the `te_mean_*` of `rep_eval`) and `testset_metrics_cases.csv` (their per-test-case quantities, one row per test case, the `te_*_set` of `rep_eval`, e.g. `te_x_ego_set`); the same for the original controller in `testset/original_metrics.json` and `testset/original_metrics_cases.csv`; and `testset/metrics_summary.csv` (one row per run). Runs that already have both files are skipped.
- Checked on the original controllers (MATLAB R2026a): the metrics of ACC and WT agree with the `te_mean_*` of OPERA's `analysis/RQ1/*_Bef_Eval_Result.mat` within 3e-13, those of MC with the executions recorded in its test set within 1e-14 and with `MC_rep_eval.m` within 4e-11. AFC is simulated in accelerator mode with a variable-step solver, whose results depend on the MATLAB release.

**Requirements.** MATLAB with Simulink and the Deep Learning Toolbox, the MATLAB Engine for Python (Step 0), and [Breach](https://github.com/decyphir/breach).
Pass the Breach folder with `--breach_path`, or set the environment variable `ISAR_BREACH_PATH`.
Otherwise a folder named `breach` next to this repository is used, e.g. `~/git3/breach` for a clone in `~/git3/ISAR`. The scripts stop before starting MATLAB if that folder does not contain `InitBreach.m`.

**How it works.** ISAR runs in Python as for UUV and MC, and only the simulation is done in MATLAB (`simulink_interface.py` and `simulink/`):
- Simulations use fastRob from AutoPatch (`simulink/fastrob/`): each MATLAB worker loads the model once and keeps it in fast restart, and candidate parameters are written into the Constant blocks of the network without recompiling the model.
- All weights and biases are perturbed, as for UUV and MC (`--perturb_biases=0` perturbs weights only). They are written with `%.17g`, so writing the original parameters leaves the model numerically unchanged.
- All test cases of one evaluation (the bad states of the region and all protected good states) are simulated in one batch, split over `--workers` MATLAB workers.

**Notes.**
- Use at most as many workers as there are free cores.
- MATLAB R2021a can deadlock inside `sim()`, in the Simulation Data Inspector, which would hang a run without any error (observed once in about 21,000 batches). Every call to MATLAB therefore has a timeout (`--eval_timeout`, 900 s by default; the first batch of a run also builds the simulation sessions and takes longer than the others). On a timeout, MATLAB and its workers are killed and restarted, and the same batch is simulated again, at most 3 times. Simulation is deterministic, so the result is the same as without the hang, and the batch is counted once. The number of restarts is recorded as `matlab_restarts` in `simulation_count.json`.
- fastRob leaves model copies (`fr_*.slx`) in the run folder and in Breach's `Ext/ModelsData` folder. `run_benchmark.py` deletes the copies in the run folder when a run finishes. On Linux, fastRob deletes the copies of finished processes in `Ext/ModelsData`; on macOS they accumulate and can be deleted by hand when no run is active.
- fastRob decides that a copy in `Ext/ModelsData` belongs to a finished process by its process id on the current machine. Jobs on different nodes that share one Breach folder can therefore delete each other's copies while they are in use. Give each concurrently running job its own copy of Breach, as AutoPatch's `run_experiment.pjm.sh` does.
- MATLAB sessions that start parallel pools at the same time on one machine can conflict in the default job storage folder. `initParpool` uses the environment variable `MATLAB_JOB_STORAGE_LOCATION` if it is set, so set it to a different folder for each job.
- The models were saved with MATLAB R2021a. Newer releases can open them; do not save them with a newer release, or R2021a can no longer open them.

### Acknowledgments

We thank Pengyuan Lu, Matthew Cleaveland, Oleg Sokolsky, Insup Lee, and Ivan Ruchkin, the authors of [Repairing Learning-Enabled Controllers While Preserving What Works](https://arxiv.org/abs/2311.03477), for making their [ISAR implementation](https://github.com/ericlupy/isar_rep) publicly available.

This repository is maintained solely to reproduce their code and use ISAR as a baseline in the OPERA experiments. Credit for the ISAR algorithm and its original implementation belongs to the original authors. Our additions support benchmark integration and experimental evaluation. Please cite the original paper when using ISAR.
