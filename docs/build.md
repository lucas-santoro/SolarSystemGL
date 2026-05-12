# Build guide

## Supported configurations

**Windows (tested)** — Visual Studio 2022 + MSVC v143. Uses the vendored `third_party/glfw/lib-vc2022/glfw3.lib`.

**Linux / macOS (path exists, not regression-tested)** — `CMakeLists.txt` falls back to `find_package(glfw3)` + `find_package(OpenGL)` when `WIN32` is false. You need a system or vcpkg install of glfw3:

```bash
# Ubuntu/Debian:
sudo apt install libglfw3-dev libgl1-mesa-dev

# macOS via Homebrew:
brew install glfw
```

GLM is header-only and stays vendored on every platform.

**Common to all:**
- CMake ≥ 3.16
- GPU with OpenGL 3.3 core profile support

## Build steps

```powershell
# from the project root
cmake -S . -B build
cmake --build build --config Release
build\Release\SolarSystemGL.exe
```

Debug build:
```powershell
cmake --build build --config Debug
build\Debug\SolarSystemGL.exe
```

Shaders are copied next to the executable as a `POST_BUILD` step (CMake `$<TARGET_FILE_DIR:SolarSystemGL>/shaders`). You can launch the exe by double-click from Explorer — it does not need a specific CWD.

## What gets linked

| Dependency | Location | Purpose |
|---|---|---|
| `glad` | `third_party/glad` | OpenGL function loader |
| `GLFW` | `third_party/glfw/lib-vc2022/glfw3.lib` | Window, context, input |
| `glm` | `third_party/glm` (header-only) | Math (vec/mat/quat) |
| `ImGui` | `third_party/imgui` + backends | Immediate-mode UI |
| `opengl32` | system | OpenGL ICD |

`CMakeLists.txt` uses an explicit source list (`PROJECT_SOURCES` + `THIRD_PARTY_SOURCES`). Adding a new `.cpp` file means editing the list — but in return, CMake always sees the right set of files without a manual reconfigure dance.

## Common issues

**"Failed to open shader file: shaders/VertexShader.glsl"** — you're running the exe from a directory that doesn't have a `shaders/` subfolder. Either run from `build/Release/` (where the POST_BUILD copies them) or from the project root after rebuilding.

**LNK2019 unresolved external on `glfw*`** — the linker can't find `glfw3.lib`. Check that `third_party/glfw/lib-vc2022/glfw3.lib` exists. If you're using a non-VC2022 toolset, this build configuration won't work — see TODO P1.

**Slow first launch** — none expected; if the window takes >1 s to appear, check console output (or stderr). `Shader::readFile` now logs missing files explicitly.

**White grid and white planet** — shaders failed to compile or load. Run from a terminal to see `stderr`; missing-shader and compile-error messages are both printed.

## Static analysis

The project builds clean under MSVC `/W4 /permissive-`. Pre-existing C4459 warnings ("declaration of X hides global declaration") in `main.cpp` parameters `camera` and `deltaTime` are tracked in P1 (eliminate globals into a `struct App`).
