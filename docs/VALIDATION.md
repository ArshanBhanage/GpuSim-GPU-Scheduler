# Validation

Measured on 2026-11-12, macOS arm64, AppleClang 17, CMake Release build.

Core build and CTest passed locally. Test checks remain active in Release builds.

Policy comparison on the included workload:

```json
{"policy": "priority", "jobs": 4, "makespan": 19.0, "mean_wait": 6.5}
{"policy": "backfill", "jobs": 4, "makespan": 19.0, "mean_wait": 3.0}
```

CUDA profiling was not run: no NVIDIA CUDA toolkit/device on this host.
