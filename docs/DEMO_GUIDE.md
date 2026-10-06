# FireScope presentation demo

This guide follows sections 59–60 of the specification. No college, team or guide
identifiers are included. CUDA will be compiled and validated on the NVIDIA PC
later; use only the existing CPU/Metal evidence until those checks pass.

## Prepare locally

Build Release and run the tests with actual GPU access. Launch the Mac demo at
512², as required by the specification:

```sh
./build/FireScope.app/Contents/MacOS/FireScope --backend metal --grid 512 --seed 42
```

Keep 1024² optional. Close the cell inspector for timing comparisons. Confirm the
window and controls are readable on the presentation display. Have these files
available offline: report and poster under `output/`, the benchmark plots under
`benchmarks/results/mac-plots/`, the screenshots under `docs/images/`, and the local
source. `build/FireScope-source.zip` contains the tracked source and deliverables.
The backup demonstration video has not yet been recorded.

## Seven-minute sequence

| Time | Action | Explanation |
| --- | --- | --- |
| 0:00–0:40 | Show the forest paused; orbit and zoom. | The grid computes wildfire spread; OpenGL displays terrain, instanced trees and visual particles. |
| 0:40–1:30 | Step, then briefly Play and Pause. | Unburned cells ignite probabilistically; burning cells consume fuel and become burned. Double buffering keeps updates synchronous. |
| 1:30–2:20 | Reset seed 42, change wind direction/speed, and run again. | Wind changes source-to-target ignition probability. Reset preserves the current environment controls, so explicitly restore them before another experiment. |
| 2:20–3:10 | Pause, select Water and paint a wet patch; show the moisture overlay. | Water lowers ignition probability through stored moisture. It does not immediately extinguish burning cells. Vegetation at zero clears fuel; positive values regrow eligible cleared cells. |
| 3:10–4:30 | Reset, Capture a scenario, apply Demo firebreak and Compare mitigation. | The two branches start from the same snapshot and run the same number of updates. Read the displayed burned-cell counts. |
| 4:30–5:30 | Show the CPU/Metal plots. | The 512² result is 45.31× for the measured headless step API workload. This excludes rendering and final GPU readback; CPU count scanning is included. CUDA results are pending. |
| 5:30–6:30 | Show `src/backends/cuda/wildfire_cuda.cuh` and `src/backends/metal/wildfire.metal`. | Each GPU thread handles one cell; the plain CUDA source uses 16×16 blocks. Shared-memory tiling remains the next experiment after NVIDIA validation. |
| 6:30–7:00 | Show the conclusion and limitations. | Synthetic terrain, simplified suppression and an uncalibrated cellular model make this an educational prototype. |

## Reproduce the report's firebreak numbers

The report's comparison is **128²**, not the 512² presentation scene. Select CPU,
128² and seed 42; restore the defaults below and Reset. Capture at timestep zero,
choose Demo firebreak, set Run steps to 500 and Compare mitigation.

| Control | Value |
| --- | --- |
| Wind speed | 3 m/s |
| Wind direction | 0° (east) |
| Humidity | 0.1 |
| Ignition probability | 0.16 |
| Moisture effect | 3 |
| Slope effect | 2 |

Observed results: baseline **16,383** burned; mitigated **11,985** burned;
**728** cleared NoFuel cells and **3,671** unburned cells in the mitigated branch.
The burned-count reduction is **26.8%**. Part of the reduction is land deliberately
cleared, so do not describe all 4,398 fewer burned cells as preserved forest.
Use Restore before trying another edit. Backend/grid changes discard snapshots.

## Short narration

“FireScope combines a cellular wildfire model with interactive 3D graphics. Every
cell examines its eight neighbors and uses wind, slope, moisture and vegetation
to calculate ignition probability. A deterministic hash lets us repeat the same
scenario and compare CPU and GPU results. The Mac CPU and Metal backends pass
the current tests. At 512², Metal's headless step API measured 45.31 times faster
than the CPU for 500 updates. A paired 128² experiment showed 26.8 percent fewer
burned cells with a firebreak. The NVIDIA source still needs validation, and the
model has not been calibrated for operational fire prediction.”

## Recording and fallback

Record the sequence above after rehearsing it on the presentation display. Capture
the controls, timestep and comparison counts; pause long enough to read them.
Keep an offline MP4, verify it plays from start to finish, and distinguish the
128² evidence scene from the 512² live scene in the narration. This is a recording
plan; it is not evidence that a video has already been produced.

If the live viewer fails, show the existing screenshots, verified benchmark plots
and report. If NVIDIA validation is unfinished, say so and show CPU/Metal only.

## Technical answers to rehearse

- **Why parallel?** Each output cell reads the same previous grid and writes its
  own next value. Cells can be processed independently within one update.
- **Why double buffering?** Reading and writing one array in place would make
  results depend on execution order and let newly ignited cells affect the same step.
- **Why Structure of Arrays?** Threads accessing the same property of adjacent
  cells read neighboring memory locations; fields can be transferred separately.
- **Why 16×16?** It is a practical starting tile of 256 threads for a 2D grid,
  not a demonstrated optimum. It still needs NVIDIA measurement.
- **Where can divergence occur?** Neighbor eligibility and transitions for
  NoFuel, Unburned, Burning and Burned cells follow different branches.
- **What is a halo tile?** A shared tile includes the surrounding one-cell border
  needed for eight-neighbor reads. Loading and synchronization can cost more than
  the saved reads; the optimization is not implemented yet.
- **Why deterministic random values?** Seed, coordinates and logical timestep
  identify a cell's random draw without depending on thread scheduling.
- **Why compact readback?** Rendering needs state and intensity rather than every
  simulation field. The normal payload is two bytes per cell; inspection uses a
  fuller snapshot and adds overhead.
- **What does instancing do?** One tree mesh is drawn many times with different
  transforms and cell associations, reducing draw-call count.
- **What is a billboard?** A camera-facing quad represents a flame or smoke
  particle. It is a visual approximation rather than a fluid simulation.
- **How does picking work?** A mouse position becomes a world-space ray by
  reversing projection/view transforms; the ray intersects the actual terrain
  triangles before a cell is edited.
- **How is speedup measured?** Divide same-host CPU median batch time by GPU
  median batch time, with the same scenario and update count. Keep timing scope
  and count-handling differences explicit.
- **How does a firebreak work?** NoFuel cells cannot ignite, interrupting local
  neighbor propagation. This model omits fire spotting across the barrier.
