# TODO

> Living backlog. **P0** = quick wins / blockers (≤30 min each).
> **P1** = core improvements that move the project forward.
> **P2** = roadmap features and long-term experiments.
>
> _Last reviewed: 2026-05-11_

---

## P0 — Quick wins & fixes

### Repo hygiene
- [x] Add `LICENSE` at root (MIT)
- [x] Add `CONTRIBUTING.md` at root
- [x] Add `README.md` at root
- [x] `git rm --cached imgui.ini`
- [x] Extend `.gitignore` to `build*/`
- [x] Delete stale `build-a/` directory

### README accuracy (sync claims with reality)
- [x] "Velocity Verlet with adaptive timestep" → "Euler-Cromer with framerate-coupled timestep"
- [x] Remove "Cross-Platform Ready" claim (now states Windows/MSVC2022)
- [x] Remove "Target FPS: 60" (now states "uncapped")
- [x] Update folder diagram: `objects/` lists `CelestialBody` + `PlanetMesh`
- [x] Fix `timeScale`: code constant now `864'000.0f` matching 10 days exactly

### Code micro-fixes
- [x] Smoothed FPS counter (EMA α=0.1 in `UIManager::renderMainPanel`)
- [x] Delete dead `Window::run()` and `Window::processInput()`
- [x] Sun emissive uniform — `emissive=1.0` for stars brightens fragment shader output

---

## P1 — Core improvements

### Physics / numerics

- [x] **Velocity Verlet** — KDK form in `PhysicsSystem::stepOnce`. Half-kick + drift + recompute + half-kick.
- [x] **Fixed timestep with accumulator** — 240 Hz physics sub-step, decoupled from render. `MAX_ACCUM = 0.5` caps spiral-of-death.
- [x] **Document softening parameter** — clarified in `docs/physics.md` and `PhysicsSystem.h` comment ("m², added to dist²"). Rename to `SOFTEN_SQ` deferred.
- [x] **Center-of-mass correction at init** — total momentum subtracted from every body's velocity, barycenter at rest in initial state.
- [x] **Conservation diagnostics panel** — `Diagnostics` collapsing section in the main panel shows KE, PE, E, |p|, |L| with relative drift vs baseline. Reset-baseline button.

### Code / build / cross-platform

- [x] `find_package(glfw3)` + `find_package(OpenGL)` on non-Windows platforms (fallback). Windows keeps the vendored `glfw3.lib` (tested baseline). GLM stays header-only / vendored on all platforms.
- [x] Replace `strncpy_s`/`_TRUNCATE` (MSVC-only) with `std::snprintf` for portability
- [x] Replaced `file(GLOB_RECURSE)` with explicit `PROJECT_SOURCES` + `THIRD_PARTY_SOURCES` lists. New `.cpp` files require an edit to `CMakeLists.txt` but no more silent missing-symbol link errors from forgotten reconfigure.
- [x] `Shader`: cache uniform locations in `std::unordered_map`, lookup on miss only
- [x] `Shader`: fail loud on compile error — throws `std::runtime_error` with shader log
- [x] Rule of 5 on `Grid` and `Shader` — both delete copy, define move; handles transferred safely
- [x] Eliminate `main.cpp` globals — `struct App` + `glfwSetWindowUserPointer`; callbacks fetch via `appFromWindow(w)`. Resolves C4459 warnings.
- [x] CI via GitHub Actions — Windows (MSVC desktop) + Linux (Emscripten web) builds, clang-format check, Doxygen. See `.github/workflows/ci.yml`.
- [x] `.clang-format` at root — Allman braces, left-aligned pointers/refs, 4-space indent, 110-col limit. Intended for format-on-save; no bulk reformatting yet.
- [ ] Optional pre-commit hook (deferred — `.clang-format` alone is enough to start)

### Visual / render

- [x] **Lambertian + Blinn-Phong lighting** — normals derived in shader from `normalize(aPos)` (no extra attribute). `lightPos = bodies[0].renderPosition()`, `viewPos = camera.getPosition()`. Stars (`emissive=1.0`) bypass lighting.
- [ ] **Texture mapping** — UV coords on `PlanetMesh`, `stb_image` loader (needs vendoring), sampler2D in fragment shader. NASA-issued planet textures are creative commons. Blocked: need stb_image.h + texture assets.
- [x] **Sun halo (fake bloom)** — additive billboarded quad with radial falloff. No framebuffer needed. Real Gaussian-blur bloom deferred to P2 — the halo delivers ~80% of the visual at ~20% of the code.
- [x] **Skybox / starfield (procedural)** — hash-based stars rendered via fullscreen quad with inverted view+projection in fragment shader. Two tiers (bright sparse + dim filler) with subtle blue↔yellow tint variation. Reuses the existing halo VAO. No cube-map / textures needed.
- [x] **Orbit trails** — `std::deque<glm::vec3>` per body (cap 512, push if moved > 0.2 WU, clear on teleport jump > 100 WU). Line-strip render with quadratic alpha fade tail → head. UI toggle in main panel.
- [x] **Saturn rings** — procedural triangle-strip ring per body with `hasRings = true`; radial banding + edge fade in fragment shader. Flag is persisted in the save format. (See P5 Done.)
- [x] **Hover highlight** — `highlight = true` scales the model matrix by 1.1× for a visible pop.
- [x] **Sun visualScale hack** → `CelestialBody.displayScale` field (data-driven). String-compare branch removed; Sun's 0.4× now lives in the init table.

### UX / interaction

- [x] Keyboard shortcuts: `Space` (pause), `R` (reset camera) — edge-detected polling, suppressed when ImGui has keyboard focus
- [x] More shortcuts: `F` (smooth-focus on selected), `Esc` (deselect / close), `1` (FREE mode), `2` (ORBITAL mode on selected)
- [x] "Reset Camera" button in the Solar System panel (`Camera::reset()`)
- [x] Color picker + name input + mass field in Add Planet
- [x] "Remove Planet" button in Planet Info — deferred mutation; camera index fixed up via `Camera::shiftOrbitalIndexOnRemove`
- [x] **Save / Load** — custom INI-like text format in `src/core/SaveLoad.{h,cpp}`. Persists `bodies` + `timeScale`. Atomic-swap on load (parser failure leaves state intact). UI: filename input + Save/Load buttons + status line in main panel. On load, camera resets to FREE and diagnostic baseline invalidates.
- [x] VSync toggle (`glfwSwapInterval(0|1)`) in main panel
- [x] **Preset systems dropdown** — `ImGui::BeginCombo` lists 3 bundled presets. Each entry calls `loadSimulation` on `presets/<name>.txt`. CMake POST_BUILD copies `presets/` next to the exe. Bundled: Solar System (default), Sun only (blank slate), Binary stars.
- [x] Time-scale quick presets — buttons for `1×`, `1k×`, `100k×`, `1M×` beside the slider

### Documentation rewrite (from scratch)

**Strategy:** new docs written; obsolete `docs/*` files (api-reference, build-guide, documentation-index, grid, shader-reference, technical-reference, README) **pending user confirmation** to delete.

- [x] `README.md` (root)
- [x] `LICENSE` (MIT)
- [x] `CONTRIBUTING.md`
- [x] `CHANGELOG.md` (Keep a Changelog format)
- [x] `docs/architecture.md`
- [x] `docs/build.md`
- [x] `docs/physics.md` (overwrote old)
- [x] `docs/render.md`
- [x] `docs/user-guide.md` (overwrote old)
- [x] `docs/api/CelestialBody.md`
- [x] `docs/api/PlanetMesh.md`
- [x] `docs/api/PhysicsSystem.md`
- [x] `docs/api/Camera.md`
- [x] `docs/api/Grid.md`
- [x] `docs/api/UIManager.md`
- [x] `docs/api/Shader.md`
- [x] `docs/adr/0001-celestialbody-unified-state.md`
- [x] `docs/adr/0002-orbital-target-by-index.md`
- [x] `docs/adr/0003-apply-button-vs-live-edit.md`
- [x] Delete obsolete `docs/{api-reference,build-guide,documentation-index,grid,shader-reference,technical-reference,README}.md`
- [ ] Inline `///` doc comments on public headers, only where intent isn't obvious from names

---

## P2 — Features & long-term

### Bodies
- [ ] Asteroid belt (10 k+ bodies — likely needs Barnes-Hut or octree to avoid N²)
- [ ] Comets on eccentric orbits (e > 0.5)
- [ ] Binary / multi-star systems
- [ ] Trappist-1 preset (7 planets in resonance)

### Physics visualization
- [ ] Lagrange points (L1–L5) markers
- [ ] Roche limit visualization
- [ ] Hill sphere overlay
- [ ] Toggleable force / velocity vectors per body

### Performance
- [ ] Barnes-Hut tree for scaling past ~50 bodies
- [ ] Structure-of-Arrays layout for `bodies` (separate pos/vel/mass arrays for vectorization)
- [ ] GPU compute shader for integration step

### Platform
- [x] **WebAssembly build via emscripten** (run in browser) — WebGL2/GLES3, GLFW3 port, rAF loop, preloaded assets, custom HTML shell.
- [x] **Browser-persistent saves (IDBFS)** + export/import. `src/core/WebPersistence.{h,cpp}`.
- [x] **Mobile / touch support** — touch gestures, responsive UI (`src/ui/UI.{h,cpp}`), action-bar overflow, MSAA quality toggle.
- [x] **CI/CD** — `ci.yml` (lint + desktop + web + doxygen) and `deploy.yml` (Vercel prebuilt). Replaces the gh-pages workflow.
- [x] **Doxygen API reference** served at `/docs`.
- [ ] Linux build validated end-to-end
- [ ] macOS build (glfw3 + OpenGL 3.3 still supported)

### Quality
- [ ] Unit tests — energy conservation after N orbits (catch2 or gtest)
- [ ] Benchmark suite — frame time vs body count
- [ ] Profiling with Tracy

### Polish
- [ ] i18n (PT-BR / EN string tables)
- [ ] Custom dark / light ImGui theme
- [ ] Splash screen
- [ ] Headless CLI mode — run N seconds, dump CSV of positions

---

## P5 — Code review checkpoints

> Tied to execution milestones. Each tick gates progression to the next phase.

- [x] **After P0** — build clean, only pre-existing C4459 warnings (globals-shadowing in `main.cpp` parameters, tracked under "eliminate globals" in P1)
- [x] **After Verlet + fixed timestep** — build clean; algorithm matches KDK reference (Half-kick / drift / recompute / half-kick); no caching across calls (correct under UI teleports); accumulator capped at 0.5s
- [x] **After lighting** — shader compiles, `lightPos` / `viewPos` set per frame, emissive bypass correct, fragment math sound; texture pipeline still pending (P1)
- [x] **After docs rewrite** — 16 new files cover architecture, build, physics, render, user-guide, 7 API class refs, 3 ADRs; 7 legacy files removed; internal links use relative paths and resolve in the new tree
- [ ] **Final audit pass** — security review, leak detection (RenderDoc / Valgrind on Linux), undefined-behavior scan (`-fsanitize=undefined`), perf profile (Tracy or built-in timer)

---

## Done (recent iteration)

- [x] Unify `Planet` + `BodyState` into a single `CelestialBody` struct
- [x] Extract GL state to `PlanetMesh` with proper Rule of 5
- [x] "Apply Changes" now writes to the physics state — edits actually take effect
- [x] "Add Planet" inserts into the same vector physics iterates → new planets gravitate
- [x] Add Planet positions at 1.5 AU with circular orbital velocity, staggered by angle, auto-selected
- [x] Play / Pause checkbox + logarithmic time-scale slider in the main panel
- [x] `Camera` orbital target tracked by index instead of pointer — resolves dangling pointer when bodies vector grows
- [x] `CMakeLists.txt` copies shaders to `$<TARGET_FILE_DIR:SolarSystemGL>/shaders` so the exe runs from `Release/`
- [x] `Shader::readFile` fails loud on missing file instead of silently returning empty string
- [x] **P0 hygiene**: `LICENSE`, `CONTRIBUTING.md`, root `README.md`, `.gitignore` extended to `build*/`, `imgui.ini` untracked, stale `build-a/` removed
- [x] **P0 README accuracy**: Verlet→Euler-Cromer (now Verlet for real), cross-platform claim removed, FPS target removed, folder diagram updated, `timeScale` value corrected to 864 000
- [x] **P0 code micro-fixes**: smoothed FPS (EMA α=0.1), `Window::run()` deleted, `emissive` uniform for star bypass in fragment shader
- [x] **Velocity Verlet** integrator (KDK form)
- [x] **Fixed-timestep accumulator** at 240 Hz
- [x] **Lambertian + Blinn-Phong lighting** with `lightPos` / `viewPos` uniforms
- [x] **Docs rewrite**: 16 new files across `docs/`, `docs/api/`, `docs/adr/`; `CHANGELOG.md` started; 7 legacy files removed
- [x] **Rule of 5** on `Shader` and `Grid` — copy deleted, move defined, handles transferred safely
- [x] **Shader uniform cache** (`std::unordered_map<string,int>`) + **fail-loud compile/link errors** (throws `std::runtime_error` with GL log)
- [x] **Globals eliminated** — `main.cpp` now uses `struct App` + `glfwSetWindowUserPointer`. C4459 warnings gone.
- [x] **`strncpy_s` → `std::snprintf`** for portability
- [x] **Center-of-mass correction** at init — barycenter at rest, total momentum exactly 0
- [x] **Hover highlight**, **`displayScale` field** (Sun string-compare hack removed)
- [x] **UX batch**: VSync toggle, Reset Camera button, hotkeys (Space, R), time-scale presets, color picker + name + mass in Add Planet, Remove Planet button (with Camera index fixup)
- [x] **Conservation diagnostics panel** — KE / PE / E / |p| / |L| with relative drift vs baseline
- [x] **Orbit trails** — deque per body, line-strip render with alpha fade, UI toggle
- [x] **Sun halo (fake bloom)** — additive billboarded quad per emissive body, no FBO needed
- [x] **Save / Load** custom text format — `src/core/SaveLoad.{h,cpp}`, atomic-swap on load, filename input + Save/Load buttons in main panel
- [x] **Preset systems dropdown** — 3 bundled presets (Solar System, Sun only, Binary stars) loaded from `presets/*.txt`. CMake POST_BUILD copies the directory next to the exe.
- [x] **More hotkeys** — F (focus), Esc (two-stage deselect / close), 1 (FREE), 2 (ORBITAL on selected). All edge-detected and ImGui-focus-aware.
- [x] **Procedural starfield** — hash-based stars in a fullscreen quad. Two brightness tiers, slight color variation. No assets.
- [x] **Saturn rings** — `hasRings` flag in `CelestialBody`, persisted in save format. Procedural triangle-strip ring mesh, radial banding + edge fade fragment shader.
- [x] **Explicit CMake source list** — `PROJECT_SOURCES` + `THIRD_PARTY_SOURCES`. No more `file(GLOB_RECURSE)`.
- [x] **`find_package` fallback** on non-Windows — `find_package(glfw3)` + `find_package(OpenGL)`. Windows keeps the vendored `.lib`.
- [x] **`.clang-format`** at repo root.
- [x] **`docs/api/SaveLoad.md`** — format spec, atomic-swap semantics.
