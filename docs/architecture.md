# Architecture

A 1000-foot view of how the pieces fit together.

## Big picture

SolarSystemGL is a single-threaded OpenGL 3.3 application. Each frame it does four things:

1. **Update physics** — N-body Newtonian gravity with Verlet integration at a fixed 240 Hz sub-step.
2. **Render bodies** — for each `CelestialBody`, upload model matrix + uniforms and draw the subdivided icosahedron.
3. **Render grid** — XZ-plane line grid distorted in y by a per-vertex mass-weighted falloff.
4. **Render UI** — Dear ImGui panels: hover label, Planet Info, Solar System control panel.

```
        ┌────────────────────────────────────────────┐
        │  main loop (src/main.cpp)                  │
        └────────────────────────────────────────────┘
            │                                  │
            ▼                                  ▼
┌──────────────────────┐         ┌──────────────────────────┐
│  PhysicsSystem        │ writes │  std::vector<            │
│  (Verlet + fixed dt)  │───────▶│   CelestialBody> bodies  │
└──────────────────────┘         └──────────────────────────┘
                                     │           ▲
                              reads  │           │ edits
                                     ▼           │
                                ┌──────────┐  ┌──────────┐
                                │  Render  │  │ UIManager│
                                │  (mesh   │  │ (ImGui)  │
                                │  + grid) │  └──────────┘
                                └──────────┘
```

## The unification

The pre-`CelestialBody` codebase carried two parallel vectors: a visual `std::vector<std::shared_ptr<Planet>>` and a physics `std::vector<BodyState>`. They were kept in sync by a one-way loop in `main.cpp`. UI edits hit the visual side only, and the sync loop silently overwrote them every frame — Apply Changes did nothing.

Now there is one struct:

```cpp
struct CelestialBody {
    // physics (authoritative, double precision)
    glm::dvec3 pos_m, vel_m;
    double     mass_kg;

    // identity
    std::string name;
    glm::vec3   color;
    float       emissive;     // 1.0 = star, 0.0 = lit body

    // visual derived
    float       density, radius;
    PlanetMesh  mesh;

    glm::vec3 renderPosition() const;        // pos_m / METERS_PER_WU
    bool      intersectsRay(...) const;
    void      recalculateGeometry();
    void      render(Shader&, bool highlight) const;
};
```

Physics, UI, render, and picking all see the same memory. The bug class that produced the original "Apply does nothing" failure is no longer representable.

See [ADR 0001](adr/0001-celestialbody-unified-state.md) for the long version.

## Module map

| Module | Responsibility |
|---|---|
| [`core/Constants.h`](api/Shader.md) | Unit conversions: `AU`, `METERS_PER_WU`, `AU_WU` |
| [`core/Window`](api/Camera.md) | GLFW window + context bootstrap |
| [`core/Camera`](api/Camera.md) | FREE / ORBITAL modes, view matrix, smooth move, ray cast |
| [`core/Shader`](api/Shader.md) | GLSL compile / link, uniform helpers |
| [`core/Grid`](api/Grid.md) | Spacetime grid mesh + per-frame mass uniforms |
| [`objects/CelestialBody`](api/CelestialBody.md) | Single source of truth for a body (physics + identity + visual) |
| [`objects/PlanetMesh`](api/PlanetMesh.md) | GL resources (VAO/VBO/EBO) with Rule of 5 |
| [`physics/PhysicsSystem`](api/PhysicsSystem.md) | Verlet integrator + fixed-timestep accumulator |
| [`ui/UIManager`](api/UIManager.md) | ImGui panels, picking, edit buffer |
| `main.cpp` | Owns the loop, the bodies vector, the camera, the input callbacks |

## Unit system

| Layer | Position | Velocity | Mass |
|---|---|---|---|
| Physics state (`CelestialBody.pos_m / vel_m / mass_kg`) | meters (double) | m/s (double) | kg (double) |
| Render space (model/view/projection) | world units (1 WU = 10⁹ m, float) | — | — |
| UI display (Planet Info) | WU (float) | km/s (float) | kg (float) |

Conversions are explicit and live in:
- `CelestialBody::renderPosition()` — meters → WU
- `UIManager::renderPlanetInfo` Apply path — WU → meters, km/s → m/s
- `UIManager::renderPlanetInfo` edit-buffer populate path — the inverses

## Threading

Single-threaded. The render thread does everything, including physics. The fixed-timestep accumulator (`PhysicsSystem.cpp:update`) decouples the **physics rate** (240 Hz) from the **render rate** (uncapped), but they share the same thread.

If you spawn a physics thread later, the body vector becomes shared mutable state and needs synchronization. Mention it in an ADR if you do.

## Lifetimes

`bodies` is `std::vector<CelestialBody>` owned by `main()`. It is passed by reference everywhere — never copied. `bodies.reserve(16)` is called at startup to keep storage stable up to 16 bodies (prevents reallocations during "Add Planet"). Camera holds the orbital target by **index**, not pointer, so even after a hypothetical reallocation the target is still resolved correctly.

`PlanetMesh` owns GL handles and is movable but non-copyable. That makes `std::vector<CelestialBody>` work correctly when it grows: each `CelestialBody` (and its `PlanetMesh`) is move-constructed into the new buffer, transferring GL handles without re-uploading geometry.

## Why "Apply" instead of live editing

We considered live-editing fields (every keystroke writes into the body). We kept the Apply button because the simulation runs while the UI is open, so live editing a position would have the planet drift under the user's hands. Apply gives a clean atomic update. See [ADR 0003](adr/0003-apply-button-vs-live-edit.md).
