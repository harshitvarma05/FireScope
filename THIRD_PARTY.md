# Third-party dependencies and references

Dependencies are fetched at pinned release tags into the build directory. Keep
their upstream license files and notices when redistributing builds or source.

| Dependency | Version | License | Use |
|---|---|---|---|
| [GLFW](https://github.com/glfw/glfw) | 3.4 | zlib/libpng | Window, OpenGL context, input |
| [GLM](https://github.com/g-truc/glm) | 1.0.1 | MIT or Happy Bunny | Graphics mathematics |
| [Dear ImGui](https://github.com/ocornut/imgui) | v1.91.9b | MIT | GLFW/OpenGL3 user interface |
| [Matplotlib](https://matplotlib.org/) | 3.10.3 | Matplotlib license (PSF-based) | Optional benchmark graph generation |

## References

- Alexandridis et al. (2008), *A cellular automata model for forest fire spread
  prediction: The case of the wildfire that swept through Spetses Island in
  1990*, DOI: https://doi.org/10.1016/j.amc.2008.06.046.
- https://github.com/Vinnu124/WildFire-Simulation-IPC
- https://github.com/anthonymansur/CIS565-Final-Project
- https://github.com/herewegoblueno/team-wildfire

These references are listed by the project specification. No source from those
wildfire repositories has been copied. Their licenses have not yet been verified.
The simulation is an original implementation of the simplified equations in
`docs/PROJECT_SPEC.md`, rather than a reproduction or validation of the paper.

## Team contributions

The repository's backend interface, CPU implementation, tests, rendering, and
application glue are authored for FireScope. Add any future adaptations here,
including upstream revision, license, copied files, and modifications.

Plotting dependencies are recorded in scripts/requirements-lock.txt. Preserve
licenses from the installed packages when distributing the plotting environment.
CUDA and Metal SDKs/toolchains are platform-provided dependencies, not vendored
source; their compute kernels and integration here were authored for FireScope.
