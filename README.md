# SolarSystemGL

A real-time 3D solar-system simulation in OpenGL 3.3 with Newtonian N-body gravity and interactive editing.

![C++](https://img.shields.io/badge/C%2B%2B-17-green) ![OpenGL](https://img.shields.io/badge/OpenGL-3.3-blue) ![Platform](https://img.shields.io/badge/Platform-Windows%20%2F%20MSVC2022-lightgrey) ![License](https://img.shields.io/badge/License-MIT-yellow)

## What it does

- Simulates the nine major bodies of the solar system (Sun + 8 planets) with real masses, distances and orbital velocities.
- Integrates gravity in double precision; renders in single precision via a 1 WU = 10⁹ m unit conversion.
- Click any body to inspect it; edit mass / density / position / velocity and watch the orbit change.
- Add new bodies on the fly — they participate gravitationally from the next physics step.
- Play / pause and logarithmic time-scale slider (1× to ~5 × 10⁶×).
- Visual grid distorted by each body's mass to evoke spacetime curvature.

## Build (Windows / MSVC2022)

```bash
cmake -S . -B build
cmake --build build --config Release
build\Release\SolarSystemGL.exe
```

Requirements:
- Visual Studio 2022 (the build links a vendored `glfw3.lib` compiled for VC2022).
- CMake ≥ 3.16.
- GPU with OpenGL 3.3 support.

Linux / macOS aren't currently supported — see [`TODO.md`](TODO.md) → P1 / Code, build, cross-platform.

## Controls

| Input | Action |
|---|---|
| `W` `A` `S` `D` | Move (free camera) |
| Right-click + drag | Look around |
| Scroll wheel | Zoom (orbital) / change speed (free) |
| Left-click on a body | Smooth-focus / set orbital target |
| Planet menu | Jump to a named body |

## Project layout

```
src/
├── core/          Camera, Window, Shader, Grid, Constants
├── objects/       CelestialBody (physics+identity), PlanetMesh (GL state)
├── physics/       PhysicsSystem (N-body integrator)
├── ui/            UIManager (ImGui panels)
└── main.cpp       Loop, init, callbacks

shaders/           GLSL 330 sources
third_party/       glad, GLFW, glm, ImGui (vendored)
docs/              In-flight rewrite — see TODO.md
```

## Status

The project is being actively cleaned up. The current state and roadmap live in [`TODO.md`](TODO.md). Highlights:

- ✅ Real-time UI editing of body properties wired into the physics
- ✅ Stable orbital-mode camera (index-based target)
- 🚧 Velocity Verlet integration (currently Euler-Cromer with framerate-coupled timestep)
- 🚧 Lighting, textures, Saturn rings
- 🚧 Cross-platform build via `find_package`

## License

MIT — see [`LICENSE`](LICENSE).

## Contributing

See [`CONTRIBUTING.md`](CONTRIBUTING.md).
