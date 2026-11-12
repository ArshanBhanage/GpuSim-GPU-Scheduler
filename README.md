# GpuSim — Multi-GPU Scheduler Simulation

A C++20 discrete-event simulator for a homogeneous GPU cluster, with Python policy comparisons and an optional CUDA vector-add profiler. No GPU is needed to run the simulator.

## Build and run

Requires CMake 3.20+, a C++20 compiler, and Python 3 for the comparison script.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

```sh
./build/gpusim examples/workload.csv 2 16 backfill
python3 python/compare.py
```

The CLI emits job start/finish times, queue wait, and assigned device IDs as CSV. The example gives mean waits of 6.5 time units under strict priority and 3.0 under backfilling; both finish at time 19. These are simulation results, not real-cluster measurements.

## Scheduling model

- Priority descends numerically, then arrival time and job ID break ties.
- Strict priority stops dispatch when the first pending job cannot fit.
- EASY-style backfilling tests lower-priority jobs against the first pending job's earliest feasible start, computed from known running-job completion times. A candidate runs only if it does not delay that reservation.
- Gang allocation places a job on distinct GPUs. Each GPU has 1.0 compute share and configurable GiB memory. First-fit placement conserves both capacities.
- Resource release happens before arrivals and dispatch at the same timestamp. Simulation jumps to the next arrival or completion.

CSV columns: `id,arrival,duration,priority,gpus,share,memory`. Arrival/duration share arbitrary consistent time units; `share` and GiB `memory` are **per GPU**. Duration must be positive and is assumed known exactly. Invalid requests, duplicate IDs, and jobs exceeding cluster capacity are rejected.

This is a SLURM-inspired educational policy model, not a SLURM implementation. No preemption, topology, network transfers, heterogeneous GPUs, runtime estimation error, or slowdown under sharing is modeled. Future higher-priority arrivals can change reservations. Fractional allocation models capacity accounting, not CUDA isolation or MIG.

## Optional CUDA profiling

On a CUDA-capable machine with toolkit installed:

```sh
cmake -S . -B build-cuda -DGPUSIM_CUDA=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-cuda --parallel
./build-cuda/cuda_profile
```

The profiler warms up a vector-add kernel, measures 100 launches using CUDA events, and verifies every output. It reports mean device time; host transfers are excluded. This independent example does not automatically calibrate simulator job durations. CUDA execution was not tested on the development Mac.

## Layout and validation

`include/scheduler.hpp` contains the reusable scheduler; `src/main.cpp` parses workloads; `tests/` checks reservation protection, strict priority, compute sharing, memory constraints, and invalid workloads. GitHub Actions builds and tests on Linux and macOS. See [validation](docs/VALIDATION.md).
