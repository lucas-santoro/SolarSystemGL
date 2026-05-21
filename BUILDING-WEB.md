# Building SolarSystemGL for the Web (WebAssembly)

The simulator compiles to WebAssembly via [Emscripten](https://emscripten.org)
and runs in any modern browser with WebGL2 support (Chrome / Edge / Firefox /
Safari 16+).

## Prerequisites

| Tool | Version tested | Notes |
|------|---------------|-------|
| Emscripten SDK | 3.x — 5.x | `emcc --version` should print a banner. |
| CMake | ≥ 3.16 | Already required by the native build. |
| Ninja | ≥ 1.10 | `emcmake` needs Ninja or `mingw32-make` — Ninja is what's used here. |
| Python | 3.10+ | Used by emsdk's helper scripts. |

### Installing Emscripten (one-time)

```bash
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
```

For each new shell, source the env so `emcc` lands in `PATH`:

```bash
# bash / zsh
source ./emsdk_env.sh

# Windows PowerShell
. .\emsdk_env.ps1
```

> **Windows + Git Bash gotcha**: if the Microsoft-Store Python stub shadows
> your real Python, set `EMSDK_PYTHON=/c/Python313/python.exe` (or wherever
> your real interpreter lives) before sourcing the env script.

> **Windows + Git Bash gotcha 2**: Bash on Windows doesn't auto-resolve `.bat`
> extensions, so you may need to invoke `emcmake.bat` / `emmake.bat` /
> `emrun.bat` instead of the bare names.

### Installing Ninja

Already installed on most CI runners. Locally on Windows:

```bash
# Option A: chocolatey (needs elevated shell)
choco install ninja -y

# Option B: download the prebuilt binary into a directory on PATH
curl -L https://github.com/ninja-build/ninja/releases/download/v1.12.1/ninja-win.zip -o ninja.zip
unzip ninja.zip -d "$HOME/bin"
```

## Configure & build

From the repo root:

```bash
emcmake.bat cmake -S . -B build-web -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-web
```

The build produces four artifacts in `build-web/`:

| File | Size | Purpose |
|------|-----:|---------|
| `SolarSystemGL.html` | ~19 KB | Default Emscripten shell. Loads the JS + wasm. |
| `SolarSystemGL.js`   | ~154 KB | Glue between the browser and the wasm module. |
| `SolarSystemGL.wasm` | ~701 KB | Compiled binary. Lazy-loaded by the JS glue. |
| `SolarSystemGL.data` | ~426 KB | Preloaded virtual FS — Inter font + every preset under `presets/`. |

Total payload: **~1.3 MB**. The `.data` file's size is dominated by Inter (411 KB).

## Running locally

Browsers won't load `.wasm` from `file://` URLs — you need a local server.
Emscripten ships one:

```bash
emrun.bat build-web/SolarSystemGL.html
```

…or any static-file server pointed at the directory:

```bash
python -m http.server -d build-web 8080
# then open http://localhost:8080/SolarSystemGL.html
```

## Differences from the native build

- **No screenshot persistence.** F12 still captures to MEMFS, but the BMP is
  written to the virtual filesystem and disappears at tab close. Right-click
  → "Save image as…" works on the canvas itself as a fallback.
- **Saves don't persist across sessions** (same reason — MEMFS only). Adding
  IDBFS-backed persistence is a future step.
- **Shaders run as OpenGL ES 3 / WebGL2.** `Shader::Shader` rewrites the
  `#version 330 core` line to `#version 300 es` + default precision
  qualifiers when `SOLARSYSTEM_BUILD_WEB` is defined.
- **`glPolygonMode` is unavailable** — `CelestialBody::render` skips the call
  on web builds (default fill mode is `GL_FILL` anyway).
- **Main loop** runs via `emscripten_set_main_loop_arg` instead of a blocking
  `while (!glfwWindowShouldClose)` so the browser stays responsive.

## Deploying

The companion workflow at `.github/workflows/web-deploy.yml` rebuilds the wasm
artifacts on every push to `master` and publishes `build-web/` to the
`gh-pages` branch, served at:

```
https://<your-github-username>.github.io/SolarSystemGL/
```

To enable: in GitHub repo settings → Pages → set the source to the `gh-pages`
branch root.
