# Measured Mac CPU/Metal results

Host: Apple M5 Pro, macOS 27.0, arm64. Release build with AppleClang 21.
See `benchmarks/results/mac.json` for run metadata and `mac.csv` for data.
These are local measurements from the implemented FireScope model.

| Grid | CPU median (ms) | Metal median (ms) | Speedup |
|---|---:|---:|---:|
| 128² | 16.714 | 2.825 | 5.92× |
| 256² | 100.039 | 4.461 | 22.42× |
| 512² | 588.152 | 12.980 | 45.31× |
| 1024² | 2893.809 | 58.224 | 49.70× |

## Method

- Same seeded forest, central ignition, environment defaults, 500 timesteps, seed 42.
- One warm-up, then five measured runs; median synchronous batch wall time.
- Restore initial data and apply ignition before each timed run.
- Exclude shader/pipeline creation, initialization, rendering and final GPU readback.
- CPU cell-count scanning occurs inside its step API; GPU counts are computed
  during the later compact readback. This table measures those implemented APIs,
  not an isolated arithmetic kernel or full interactive frame time.
- Continue all 500 timesteps even if fire finishes early on smaller grids.
- GPU elapsed time is also exposed in the viewer separately from benchmark wall time.

Final burned counts match between CPU and Metal at each size: 16,383; 65,528;
192,598; and 194,951 respectively. The correctness suite independently compares
step-by-step cell state, fire intensity and counts on small scenarios, including
wind, slope, moisture, firebreaks, reset and batching.

These measurements depend on hardware, scenario and batch length. They do not
establish speedup for every scenario or claim physical forecasting accuracy.
CUDA results and CUDA naive-versus-tiled comparisons await NVIDIA validation.

## Reproduce

```sh
python3 scripts/run_benchmarks.py --executable build/FireScope.app/Contents/MacOS/FireScope --backends cpu metal --output benchmarks/results/mac.csv
python3 -m venv .venv
.venv/bin/python -m pip install -r scripts/requirements-lock.txt
.venv/bin/python scripts/plot_results.py benchmarks/results/mac.csv --output benchmarks/results/mac-plots
```

Graphs are available as PNG and PDF: execution time, speedup, and throughput.
