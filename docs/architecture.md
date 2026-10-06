# Architecture and milestone status

```
Application / ImGui
        |
ISimulationBackend -> CPUBackend / CUDABackend / MetalBackend
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
- M3: CUDA naive kernel, device allocation, timing, compact output and brushes
  implemented; compile/runtime/CPU-parity checks and benchmarks await NVIDIA host.
- M4: Standalone Metal compute, wildfire implementation, exact tested CPU parity,
  reset/brushes, 512² 3D live run and local CPU/Metal benchmark graphs verified.
- M5: Water UI, full-state snapshots (including logical timestep), restore and
  baseline-versus-mitigation comparison verified on CPU/Metal. Demo firebreak preset.
- M6: Mac benchmark CSV/plots exist. NVIDIA results, shared-memory comparison,
  graphics polish, report, poster and backup video remain pending.

CUDA compiles only when a CUDA compiler is detected; Metal compiles only on Apple
when enabled. Both can be disabled for a dependency-free CPU headless build.
Factory discovery only offers device-supported backends. OpenMP is still optional
future work. Missing backend requests fail clearly. Measured same-host CPU/Metal
results are documented in PERFORMANCE.md; CUDA has not yet been validated.

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

## GPU implementation

CUDA uses a plain 16x16 global-memory kernel, paired device state/fuel/age buffers,
static environment fields, CUDA events and a compact uchar2 render output. Metal
uses the same algorithm with shared MTLBuffers, compute encoders, 16x16 groups,
command completion and GPU timestamps. Separate encoders establish tracked buffer
dependencies between Metal steps; CUDA default-stream ordering does the same.

GPU render packing transfers only two bytes per cell. Host counts are updated on
compact downloads; a cached stats query avoids redundant readbacks. Brush edits
execute on the selected GPU. CPU parameters and hash semantics are shared or
mirrored explicitly; the Metal uniform layout is checked with sizeof/offsetof.
The CPU owns logical timesteps; GPU dispatch copies that same counter into params.

Metal shader source is embedded by CMake and compiled into a runtime pipeline
with fast math disabled. CUDA disables FMA contraction. Those choices reduce
floating-point differences but do not prove universal bitwise equality across
all hardware; the consistency suite verifies explicit scenarios without relaxing
state/intensity assertions. GPU tests return a distinct skip code when no device
is visible. The NVIDIA checklist includes memcheck/racecheck.

## Scenario comparison

The snapshot owns copied state, fuel, moisture, elevation, vegetation, density,
burn ages, config, environment and elapsed timestep. Full GPU readbacks occur only
for explicit snapshot/comparison actions; the normal frame remains compact.
Restoring preserves the timestep so hash(seed,x,y,step) reproduces future draws.

Comparison initializes two independent backends from the original and edited
grids, restores the same timestep and applies the same captured parameters for
the requested number of steps. It returns baseline/mitigated counts and the final
mitigated grid for display. The source snapshot is never mutated. The UI requires
interventions at the captured timestep; advancing simulation disables comparison
until restore. Controls retain an explicit separation between Reset (seeded fresh
forest) and Restore (captured scenario).

Tests include snapshots taken after three elapsed steps, future replay, moisture
readback, identical paired runs, and a known full firebreak on a 31x21 grid: 651
baseline burned cells versus 315 mitigated burned cells, 21 cleared barrier cells
and 315 unburned cells. Those deterministic counts pass on CPU and Metal.

The viewer's seeded 128² demo firebreak was also run through the actual UI for
500 steps: 16,383 baseline burned versus 11,985 mitigated burned (26.8% reduction),
728 cleared cells and 3,671 unburned cells. Burned-count reduction includes cells
cleared for the barrier; it should not all be described as forest preserved.


## Forest and particles

The OpenGL 4.1 renderer builds an original procedural tree mesh from tapered
six-sided trunk segments, primary branches, twigs and leaf cards. Leaf fragments
use a pointed silhouette and vein shading. Each instance has a position, scale,
yaw, cell UV and color variety. One randomly selected cell within each sampling
block breaks up regular rows; roots interpolate the exact terrain triangle.
Density determines which candidates are emitted. The stride targets no more than
roughly 12,000 candidates (rounding upward reduces the count further).

The state texture removes cleared trees, progressively sheds burning foliage as
fuel decreases, and leaves bare charred branches in burned cells. Visual leaf
movement uses a separate clock and does not alter simulation fields. Branch
geometry is representative, not part of the cellular combustion calculation.

A 2048² depth map renders terrain and alpha-cut vegetation from a directional
light. The camera pass applies a 3×3 percentage-closer shadow filter, warm direct
light, cool ambient light and distance haze. Procedural ground noise blends soil,
grass and slope-dependent rock. A fullscreen sky pass adds a horizon gradient,
sun and cloud variation. Sky/shadow programs are embedded alongside the terrain
shaders, preserving working-directory-independent launches.

Flames, glow and smoke are camera-facing quads rendered in one instanced draw,
sorted by camera depth, with alpha blending, depth testing and depth writes
disabled. There are at most 2,048 flames, 2,048 glow instances and 4,096 smoke
particles. Smoke emission is limited to 500 particles/second. Smoke has position,
velocity, age, opacity and size; motion combines upward drift, wind velocity
converted from meters to cells, and sinusoidal turbulence. Noise-shaped fragment
opacity and shading produce less uniform smoke; animated turbulence creates
flame tongues. This remains a bounded particle approximation, not a volumetric
fluid solver or the reference repository's ray-marched smoke model.

Visual time does not affect the simulation's seed or timestep. Toggling particles
off clears the pool. The new **Forest view** control provides a closer, lower
orbit angle; **Reset view** restores the complete terrain framing. The tree and
particle toggles remain available for examining terrain or reducing graphics cost.

Live validation included the seeded 128² forest, manual stepping to timestep 55,
a close view of burning and charred trees, and a Metal 512² five-frame smoke check
with OpenGL error checks. All eight model/backend/geometry tests passed after the
renderer update. The preview is in `docs/images/realistic-forest-fire.png`.

The cell inspector downloads full state only when requested or while observing
steps. It evaluates the same combined neighbor probability and captures the
selected cell's probability before each observed update; it cannot reconstruct
unobserved historical probabilities. Its overhead is explicitly labeled and
excluded from the headless benchmark path.


## Vegetation and moisture tools

An additional RG32F texture stores moisture and density. It refreshes at scenario
load/restore/comparison or when a water/vegetation stroke ends. Normal frames
still download only the two-byte state/intensity payload. The moisture overlay
uses actual cell moisture to tint both ground and foliage; density also controls
the ground's grass blend. Vegetation strokes rebuild the sampled tree instances
without rebuilding terrain, changing camera position, or clearing the particle
pool. Cleared cells can visibly regrow after a positive-density edit.

The nine-test suite passes on CPU/Metal, including odd dimensions, boundary edits,
water saturation, vegetation clearing/regrowth, preserved burning/burned cells,
property persistence through buffer swaps and unchanged logical timesteps.
The live viewer was checked by painting a saturated radius-12 wet zone, clearing
a radius-12 forest patch (441 NoFuel cells), then regrowing it (NoFuel returns to
zero). The saved wet-zone/clearing preview is `docs/images/mitigation-tools.png`.
