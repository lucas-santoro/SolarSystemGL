# Changelog

All notable changes to this project will be documented here. Format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/); versions follow [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- **WebAssembly build** via Emscripten — runs in any WebGL2 browser. GLFW3 port, WebGL2/GLES3 shaders, rAF main loop, live canvas sizing, preloaded presets/fonts, custom HTML shell.
- **Browser-persistent saves (IDBFS).** `src/core/WebPersistence.{h,cpp}` mounts IDBFS at `/user` and syncs to IndexedDB so user saves survive a reload. The web Load modal lists built-in presets and user saves in two sections with per-row export/delete and an import button; overwrite is confirmed. Falls back to in-memory with a warning when IndexedDB is unavailable. No-op on desktop.
- **Mobile & touch support.** Emscripten touch callbacks (one-finger drag = orbit, two-finger pinch = zoom, tap = select); responsive UI helpers in `src/ui/UI.{h,cpp}` (viewport breakpoints, font/padding scaling, viewport-clamped modal sizing); the action bar collapses into a `⋯` overflow menu (with Demo Mode and Screenshot entries) on narrow viewports.
- **Runtime quality toggle.** `Framebuffer::setSamples` rebuilds the scene FBO so MSAA can be switched from Settings; `Platform::isMobileDevice` defaults MSAA off on phones.
- **Web screenshots.** `Screenshot` split into a pure `encodeFramebufferAsBMP` + file writer; the web build downloads the BMP via the browser instead of writing to the throwaway virtual FS.
- **Offscreen MSAA resolve** (4× multisample scene FBO blitted to a single-sample target for the composite) and **path-prediction horizon freeze** for a stable predicted trajectory in orbital camera.
- **CI/CD pipeline.** `.github/workflows/ci.yml` (clang-format check, MSVC desktop build, Emscripten web build, Doxygen) and `deploy.yml` (prebuilt static deploy to Vercel — app at `/`, Doxygen at `/docs`).
- **Doxygen API reference** (`Doxyfile`, doxygen-awesome theme) generated in CI and served at `/docs`.
- `CelestialBody.emissive` flag and matching uniform in the fragment shader. Stars skip lighting calculation and emit at brightness > 1.0.
- `CelestialBody.displayScale` field — visual-only model-matrix multiplier (Sun = 0.4 so it doesn't engulf inner orbits). Replaces the `(name == "Sun")` string-compare branch.
- Lambertian + Blinn-Phong lighting in `FragmentShader.glsl`. `lightPos` and `viewPos` uniforms set per frame from `bodies[0]` (Sun) and `camera.getPosition()`.
- Velocity Verlet integrator (KDK form) in `PhysicsSystem::stepOnce`.
- Fixed-timestep accumulator pattern in `PhysicsSystem::update`. Physics runs at 240 Hz independent of render framerate.
- `MAX_ACCUM` spiral-of-death cap (0.5 s).
- Center-of-mass correction at init: total momentum subtracted from every body, barycenter starts at rest.
- Play / Pause checkbox, logarithmic time-scale slider, and `1x / 1k× / 100k× / 1M×` quick-preset buttons.
- VSync toggle in the Solar System panel.
- Reset Camera button + `Camera::reset()` method.
- Hover highlight: bodies under the mouse are drawn at 1.10× scale.
- Add Planet now exposes Name, Color, and Mass inputs.
- Remove Planet button in Planet Info; camera orbital target index is fixed up via `Camera::shiftOrbitalIndexOnRemove`.
- Hotkeys: `Space` toggles pause, `R` resets camera. Edge-detected polling, suppressed while ImGui has keyboard focus.
- Diagnostics panel (collapsing section) shows KE, PE, E, |p|, |L| with relative drift vs a resettable baseline.
- Orbit trails: `std::deque<glm::vec3>` per body, push current position when motion exceeds 0.2 WU, teleport-clear on > 100 WU jump, line-strip render with quadratic alpha fade (`TrailVertexShader.glsl` + `TrailFragmentShader.glsl`). Toggleable.
- Sun halo (fake bloom): additive billboarded quad per emissive body (`HaloVertexShader.glsl` reconstructs camera right/up from the view matrix; fragment shader has radial quadratic falloff). Toggleable. Real Gaussian-blur bloom deferred to P2.
- Save / Load: custom INI-like text format in `src/core/SaveLoad.{h,cpp}`. Persists each body's pos/vel/mass/color/density/emissive/displayScale plus `timeScale`. `std::scientific << setprecision(15)` for double fidelity. Atomic-swap on load: parse into staged vector, only commit if non-empty. UI: filename input + Save / Load buttons + status line in the main panel. On successful load: camera → FREE, selection cleared, diagnostic baseline invalidated.
- Preset systems dropdown in the main panel, listing 3 bundled save files (`presets/solar-system.txt`, `presets/sun-only.txt`, `presets/binary-stars.txt`). CMake POST_BUILD copies the `presets/` directory next to the executable, same pattern as `shaders/`.
- Procedural skybox / starfield: `SkyVertexShader.glsl` + `SkyFragmentShader.glsl`. Fullscreen quad rendered before everything else with depth test disabled. Fragment shader reconstructs world direction from inverse view+projection and hashes a 350-cell grid to emit two brightness tiers of stars with subtle color variation. Reuses the existing halo VAO.
- Saturn rings: `CelestialBody.hasRings` flag (persisted in the save format). Procedural triangle-strip ring mesh (64 segments, inner radius 1.0, outer 1.5 in unit space) with `RingVertexShader.glsl` + `RingFragmentShader.glsl`. Fragment shader does radial banding (sin × 28) and edge fade. Default Saturn (init + preset) has `hasRings = 1`.
- More hotkeys: `F` smooth-focuses the camera on the selected body, `1` switches to FREE camera mode, `2` switches to ORBITAL on the selected body. `Esc` is now two-stage: it clears the current Planet Info selection if any; otherwise it closes the app (the old single-stage Esc=close behavior moved out of `processInput` into the edge-detected hotkey block).
- `.clang-format` at the repo root. Allman braces, 4-space indent, 110-col limit, left-aligned pointers and references. Intended for editor format-on-save, no bulk reformat performed.
- `docs/api/SaveLoad.md` — full spec for the save file format including key inventory, precision rationale, atomic-swap semantics, and caller responsibilities post-load.
- `Shader::setFloat` helper.
- Smoothed FPS counter (EMA, α = 0.1).
- Root `README.md`, `LICENSE` (MIT), `CONTRIBUTING.md`, `TODO.md`, `CHANGELOG.md`.

### Changed
- `CMakeLists.txt` switched from `file(GLOB_RECURSE)` to explicit `PROJECT_SOURCES` and `THIRD_PARTY_SOURCES` lists. New `.cpp` files now require a `CMakeLists.txt` edit, but the build no longer misses files when the generator isn't reconfigured.
- Linker logic is platform-conditional: Windows keeps the vendored `third_party/glfw/lib-vc2022/glfw3.lib` (tested baseline); other platforms fall back to `find_package(glfw3)` + `find_package(OpenGL)`. GLM stays header-only and vendored everywhere.
- Unified `Planet` + `BodyState` into a single `CelestialBody` struct (single source of truth).
- Extracted GL state into `PlanetMesh` with explicit Rule of 5 (non-copyable, movable).
- `Shader` and `Grid` likewise gain Rule of 5 — copy deleted, move defined, GL handles transferred safely.
- `Shader` caches uniform locations in an `unordered_map`. Compile and link failures throw `std::runtime_error` with the shader log instead of silently leaving an invalid program ID.
- `main.cpp` globals (camera, uiManager, lastX/Y, firstMouse, deltaTime, lastFrame) replaced by a `struct App` set via `glfwSetWindowUserPointer`. C4459 warnings resolved.
- `UIManager` uses `std::snprintf` instead of MSVC-only `strncpy_s`.
- `Camera` orbital target now tracked by index instead of pointer — eliminates dangling-pointer risk when `bodies` reallocates. New helper `shiftOrbitalIndexOnRemove` keeps the index valid after a body is removed.
- `PhysicsSystem::update` is no longer `const` (it owns the accumulator and double-buffered acceleration arrays).
- `PhysicsSystem::timeScale` corrected from 860 400 to 864 000 (10 days exactly).
- `CMakeLists.txt` copies shaders to `$<TARGET_FILE_DIR:SolarSystemGL>/shaders` so the exe runs from `build/Release/` without CWD gymnastics.
- `Shader::readFile` fails loud on missing file instead of silently returning an empty string.
- `.gitignore` extended from `build/` to `build*/`.

### Removed
- `src/objects/Planet.{h,cpp}` (replaced by `CelestialBody` + `PlanetMesh`).
- `src/physics/BodyState.h` (folded into `CelestialBody`).
- `Window::run()` and `Window::processInput()` (dead code).
- `imgui.ini` from tracked files (already ignored).
- Stale `build-a/` directory.

### Fixed
- **The big one:** edits to mass / density / position / velocity via Planet Info → Apply Changes now actually propagate to the physics. Previously, edits hit `Planet` while physics read from `BodyState`, and a sync loop overwrote position each frame.
- "Add Planet" now inserts into the same vector physics iterates → new bodies gravitate.
- Shaders are now found when the exe is launched from `build/Release/` directly.
