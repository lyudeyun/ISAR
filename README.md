# Repeatability Package of Incremental Simulated Annealing Repair

### Step 0: Prepare required packages

This is the repeatability package for ICCPS 2024 Submission "Repairing Learning-Enabled Controllers While Preserving What Works".
To run our code, `Python >= 3.7.0` needs to be installed, and then the required packages shall be installed by
```
pip install -r requirements.txt
```

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
