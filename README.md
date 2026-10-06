# FireScope

Cross-platform GPU-accelerated wildfire propagation and mitigation simulator,
for Computer Graphics/VR and Parallel Architecture/GPU Programming.

The implementation follows [the project specification](docs/PROJECT_SPEC.md).
CPU, CUDA, and Metal compute are kept separate from OpenGL rendering.

> Educational prototype, not an operational wildfire forecasting system.

## Build

Requirements: CMake 3.24+, a C++17 compiler, and OpenGL 4.1 support.
On macOS use Xcode/Command Line Tools; on Windows use Visual Studio C++ tools.
The first configuration downloads pinned GLFW, GLM, and Dear ImGui releases.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j
./build/FireScope
```

On Windows run `build/Release/FireScope.exe` with a multi-configuration generator.

## Milestones

- M0: CMake, OpenGL triangle, ImGui. Mac verification underway; Windows pending.
- M1: CPU cellular model and deterministic tests — next.
- M2–M6: terrain/picking, CUDA, Metal, mitigation, benchmarks and deliverables.

Dependencies and algorithm references are recorded in [THIRD_PARTY.md](THIRD_PARTY.md).
