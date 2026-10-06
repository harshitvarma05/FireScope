# Architecture and milestone status

```
Application / ImGui
        |
ISimulationBackend -> CPUBackend (current)
        |             CUDABackend / MetalBackend (future)
        v
RenderState (state + intensity, 2 bytes/cell)
        |
OpenGL state texture + terrain mesh -> lit fragment shader -> 3D viewport
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
- M2: Indexed 3D terrain, smooth normals, directional lighting, model/view/projection,
  depth buffering, orbit/free-fly camera, brush preview, and ray-to-triangle picking.
  Geometry tests and Mac 3D rendering checks pass.
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

## Terrain and picking

The heightfield has (width+1)*(height+1) shared corner vertices and two triangles
per cell. Corner heights average neighboring simulation elevations; displayed
height is meters divided by cellSizeMeters, keeping one horizontal world unit
equal to one simulation cell. East is +X; north is -Z. Smooth normals use local
height differences. Static vertex/index buffers upload when a scenario resets.

The renderer consumes compact cell state/intensity as a two-channel texture.
GLSL 410 source is maintained in shaders/terrain.vert and shaders/terrain.frag,
then embedded by CMake so the app bundle runs without a source-tree working
directory. Model, view and perspective projection are separate uniforms.

Picking converts viewport-relative logical mouse coordinates to NDC, unprojects
near/far clip points with inverse(projection*view), intersects a terrain bounding
box, and traverses crossed cells using a 2D DDA. Each visited cell tests its exact
two rendered triangles; the nearest hit determines the brush cell. This avoids
scanning millions of triangles for every cursor move. Tests compare 600 oblique
rays against an independent exhaustive triangle scan and cover boundaries,
parallel/vertical rays and projection/unprojection.

Logical coordinates are used for picking and scaled framebuffer coordinates for
OpenGL viewports, including Retina displays. Camera and brushes consume ImGui's
event-fed mouse coordinates consistently. Orbit/free-fly switching preserves the
current eye; pitch is limited to avoid lookAt singularities.
