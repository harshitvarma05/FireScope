# FireScope

FireScope is a cross-platform GPU-accelerated wildfire propagation and
mitigation simulator developed as a combined Computer Graphics/VR and
Parallel Architecture/GPU Programming college project.

Planned compute backends: serial CPU, NVIDIA CUDA, Apple Metal, optional OpenMP.
The CPU reference is implemented now. CUDA and Metal follow the CPU/graphics
milestones in [the specification](docs/PROJECT_SPEC.md).

> Educational prototype, not intended for operational wildfire forecasting.

## Build and run

Requirements: CMake 3.24+, a C++17 compiler, and OpenGL 4.1 support.
On macOS use Xcode/Command Line Tools; on Windows use Visual Studio C++ tools.
The first GUI configuration downloads pinned GLFW, GLM and Dear ImGui releases.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j
ctest --test-dir build -C Release --output-on-failure
```

On macOS, open `build/FireScope.app` in Finder, or run:

```sh
./build/FireScope.app/Contents/MacOS/FireScope
```

On Windows (Visual Studio generator):

```powershell
.\build\Release\FireScope.exe
```

On Linux with a suitable OpenGL installation: `./build/FireScope`.

The Mac build and GUI have been tested locally. Windows portability is designed
into the sources and CMake configuration but requires Windows validation.

## Current controls

- Play / Pause / Step / Reset.
- Seed (applies on Reset) and 128² / 256² / 512² / 1024² grid selection.
- Wind speed/direction, humidity, ignition probability and moisture/slope strengths.
- Ignite / Firebreak brush, radius 1–30. Click/drag on the grid to paint.
- Steps per frame: 1 / 2 / 4 / 8; compact render state downloaded once per frame.
- Burning, burned, unburned and no-fuel counts, timestep, timing and FPS.
- Orbit camera: right-drag to rotate, middle-drag to pan, scroll to zoom.
- Free-fly camera: right-drag to look, WASD to move, Q/E down/up.
- Reset view reframes the terrain; Grid lines toggles the cell overlay.
- A terrain cursor shows the picked cell and elevation; the ring previews brush radius.
- Escape closes the window.

Reset restores the seeded forest and the same central ignition, clears painted
edits and pauses. It retains the current environment controls. 0° wind blows east;
90° blows north. Green means unburned, orange burning, charcoal burned, brown no fuel.
The viewer now renders a lit 3D heightfield with perspective projection and depth
buffering. Ray picking intersects the actual terrain triangles before painting.

Use `--triangle` to replay the original OpenGL/ImGui skeleton.
Use `--smoke-test` to run five simulation/render frames and check OpenGL errors.

## Headless CPU build

No OpenGL or downloaded dependencies are needed for this build:

```sh
cmake -S . -B build-headless -DCMAKE_BUILD_TYPE=Release -DFIRESCOPE_BUILD_GUI=OFF
cmake --build build-headless --config Release -j
ctest --test-dir build-headless -C Release --output-on-failure
./build-headless/FireScope --backend cpu --grid 128 --steps 500 --seed 42 --headless
```

On Windows the headless executable is `build-headless/Release/FireScope.exe`.
Headless output is CSV with the columns from the specification. Initialization,
scenario generation and rendering are excluded from the timed interval. Counts
are included in simulation time. These single runs are smoke measurements;
formal benchmarking still needs warm-up, five repeats and median reporting.

## Milestone status

- M0: Mac CMake/OpenGL 4.1/ImGui triangle verified; Windows verification pending.
- M1: CPU model and deterministic tests pass; basic texture visualization works.
- M2: 3D terrain, orbit/free-fly camera, depth buffering and ray picking implemented.
- M3–M6: CUDA, Metal, scenario comparison, formal benchmarks and final deliverables remain.

See [architecture](docs/architecture.md), [model choices](docs/model.md), and
[dependencies/attribution](THIRD_PARTY.md). No source from the referenced wildfire
repositories has been copied. Repository licensing has not yet been selected by
the team; third-party dependencies retain their own licenses.
