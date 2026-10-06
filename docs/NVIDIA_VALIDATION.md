# CUDA / Windows validation

CUDA source is implemented but has not been compiled or run on the development
Mac. Do not mark M3 or Windows portability complete until these checks pass on
the team's NVIDIA computer. No CUDA performance results have been fabricated.

## Windows build

Install the Visual Studio C++ build tools, CMake, Git, an NVIDIA driver and a
CUDA Toolkit compatible with the computer's GPU and C++ toolchain, as described
in PROJECT_SPEC.md. In a developer PowerShell, from the project directory:

```powershell
nvcc --version
nvidia-smi
cmake -S . -B build-nvidia -DFIRESCOPE_ENABLE_CUDA=ON -DFIRESCOPE_ENABLE_METAL=OFF
cmake --build build-nvidia --config Release -j
ctest --test-dir build-nvidia -C Release --output-on-failure
.\build-nvidia\Release\FireScope.exe --list-backends
.\build-nvidia\Release\FireScope.exe --backend cuda --grid 512
```

`cuda` must appear in the backend list. The default CUDA architecture is the
local GPU (`native`); set `-DCMAKE_CUDA_ARCHITECTURES=...` explicitly only when
building for another supported GPU. An absent compiler falls back to CPU so the
common application still builds. A skipped GPU consistency test does not count
as CUDA validation.

## Kernel correctness and memory checking

```powershell
compute-sanitizer --tool memcheck .\build-nvidia\Release\test_backend_consistency.exe
compute-sanitizer --tool racecheck .\build-nvidia\Release\test_backend_consistency.exe
```

The test compares every step of known probability-zero/one and firebreak
scenarios, a seeded wind/moisture/slope case, brush edits, counts, reset,
batched steps and odd grid dimensions against CPU. It also runs a 512² case.
If stochastic consistency differs, inspect floating-point evaluation before
changing the assertion or claiming exact cross-device determinism.

## Benchmark on the same NVIDIA host

```powershell
python scripts/run_benchmarks.py --executable build-nvidia/Release/FireScope.exe --backends cpu cuda --output benchmarks/results/nvidia.csv
python scripts/plot_results.py benchmarks/results/nvidia.csv --output benchmarks/results/nvidia-plots
```

Plotting requires matplotlib; benchmark collection uses only Python's standard
library. Each benchmark restores the same scenario, does one warm-up and reports
the median of five measured runs. Capture the console output, CSV, metadata,
GPU/driver/toolkit version and a screenshot of the running Windows viewer.

Linux uses the same steps with a Release single-configuration build and executable
paths such as `build-nvidia/FireScope` instead of `build-nvidia/Release/FireScope.exe`.

Shared-memory tiling is a later optimization milestone. First validate and
measure this plain global-memory kernel.
