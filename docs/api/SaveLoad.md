# `SaveLoad`

`src/core/SaveLoad.h` / `.cpp`

Two free functions that persist and restore the simulation state to a human-editable text file. No third-party dependencies.

## Public API

```cpp
bool saveSimulation(const std::string& path,
                    const std::vector<CelestialBody>& bodies,
                    const PhysicsSystem& physics);

bool loadSimulation(const std::string& path,
                    std::vector<CelestialBody>& bodies,
                    PhysicsSystem& physics);
```

Both return `true` on success. `loadSimulation` is **atomic**: if parsing fails or the file contains zero `[body]` sections, neither `bodies` nor `physics.timeScale` are modified.

## File format

INI-like text, whitespace tolerant, `#` comments. One global section at the top (no header), one `[body]` section per body.

```
# free-form comment line
version = 1
timeScale = 8.640000000000000e+05

[body]
name = Sun
pos_m = 0 0 0
vel_m = 0 0 0
mass_kg = 1.989e30
color = 1.0 0.9 0.3
density = 1408
emissive = 1.0
displayScale = 0.4
hasRings = 0

[body]
name = Earth
...
```

### Keys (global)

| Key | Type | Notes |
|---|---|---|
| `version` | int | Schema version. Currently 1. Unknown values are ignored. |
| `timeScale` | float | Real-time → simulated-time multiplier. Applied on load. |

### Keys (per `[body]`)

| Key | Type | Units | Notes |
|---|---|---|---|
| `name` | string | — | Free text; can contain spaces. |
| `pos_m` | dvec3 | meters | Three whitespace-separated doubles. |
| `vel_m` | dvec3 | m/s | Three whitespace-separated doubles. |
| `mass_kg` | double | kg | |
| `color` | vec3 | linear RGB, 0–1 | |
| `density` | float | kg/m³ | Drives `radius` via `recalculateGeometry`. |
| `emissive` | float | 0 or 1 | 1.0 = star (skips lighting, gets halo). |
| `displayScale` | float | — | Visual-only model-matrix multiplier (e.g. Sun = 0.4). |
| `hasRings` | bool | `0` / `1` | Renders the procedural ring disk around this body. |

Unknown keys are silently ignored — forward-compatible.

## Precision

`saveSimulation` uses `std::scientific << std::setprecision(15)` so doubles round-trip without loss. Default `<<` precision (6 digits) would clip Earth's position from 1.495978707×10¹¹ down to 1.49598×10¹¹ — a 50 000 km error.

## Atomic-swap on load

```cpp
std::vector<CelestialBody> staged;
float stagedTimeScale = physics.timeScale;
// ... parse into `staged`, never touch `bodies` ...
if (staged.empty()) return false;

for (auto& b : staged) b.recalculateGeometry();  // rebuild GL meshes

bodies            = std::move(staged);
physics.timeScale = stagedTimeScale;
```

Two consequences:
- A malformed file or missing path leaves the running simulation untouched.
- `recalculateGeometry` (which uploads to the GPU) runs **before** the swap, so if the upload throws, you still have the old state.

## What's not persisted

Intentionally:
- **Trail points** (`CelestialBody.trailPoints`) — transient visual state, regenerates immediately.
- **Mesh GL handles** — non-portable; rebuilt by `recalculateGeometry`.
- **PhysicsSystem accumulator** — transient.
- **Camera state** — saved files focus on the *physical world*, not the user's viewing pose.
- **UI state** — `selectedPlanetIndex`, `hoveredIndex`, etc. are reset by the caller on load.
- **COM correction** — `loadSimulation` does *not* re-center momentum. What's in the file is what you get. This keeps save → load roundtrip exact; presets may have small barycenter drift.

## Caller responsibilities on load

After `loadSimulation` returns true, the body vector has been replaced. Anything that holds indices or references into it must be invalidated:

```cpp
camera.setMode(CameraMode::FREE);     // orbital target index now refers to a different body
selectedPlanetIndex = -1;             // selection / hover were indices into old vector
lastSelectedIndex   = -1;
hoveredIndex        = -1;
diagBaselineSet     = false;          // conservation baseline was for the old system
```

`UIManager` does exactly this when its Load button or preset dropdown succeeds.

## Presets

Three bundled `.txt` files live in `presets/` and are copied next to the executable by the CMake `POST_BUILD` step. The preset dropdown in `UIManager::renderMainPanel` calls `loadSimulation` on the chosen path.

Adding a new preset: drop a `.txt` in `presets/`, then add a `{ "Label", "presets/file.txt" }` entry to the `presets[]` array in `UIManager.cpp`.
