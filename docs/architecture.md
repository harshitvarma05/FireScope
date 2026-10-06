# Architecture and milestone status

```
Application / ImGui
        |
ISimulationBackend -> CPUBackend (current)
        |             CUDABackend / MetalBackend (future)
        v
RenderState (state + intensity, 2 bytes/cell)
        |
OpenGL state texture -> fragment shader -> viewport
```

Simulation code contains no graphics APIs. The renderer consumes only compact
render state; no CUDA/OpenGL interop is introduced. The CPU grid uses Structure
of Arrays. Ignite/firebreak/water edits use a common backend brush API so future
GPU backends can implement device-side edits without exposing their buffers.

The graphical application targets OpenGL 4.1 Core and GLSL 410. OpenGL functions
are loaded with GLFW; ImGui uses its own included OpenGL3 loader. GUI dependencies
are optional: a headless build needs only CMake and a C++17 compiler.

## Status

- M0: Mac OpenGL triangle and ImGui built and launched. Local commit saved.
  Cross-platform CMake source exists; Windows build/runtime still unverified.
- M1: 128² CPU spread, deterministic seed/reset, tests, wind/moisture/slope,
  state statistics, flat texture viewer, ignition and firebreak controls.
- M2: 3D terrain, camera and ray picking are next.
- M3: CUDA implementation and CPU consistency/benchmark comparison pending.
- M4: standalone Metal compute test, wildfire port and Mac benchmarking pending.
- M5: water UI, snapshots and baseline/mitigation comparison pending.
- M6: measured benchmarks/plots, report, poster and backup video pending.

CUDA/Metal/OpenMP CMake options reserve the specified configuration names; no
implementation is enabled yet. Backend CLI requests other than cpu fail clearly.
No cross-device speedup claims are made from the initial headless runs.

The larger grids are selectable for testing, but 128² remains the development
default until the GPU and 3D graphics milestones are stable. The final Mac demo
default will change to 512² at the stage prescribed by the specification.
