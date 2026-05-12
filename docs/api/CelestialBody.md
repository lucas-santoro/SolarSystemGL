# `CelestialBody`

`src/objects/CelestialBody.h`

A single source of truth for one body in the simulation. Holds physics state, identity, and visual derived state in one place. Replaces the pre-unification pair `Planet` + `BodyState`.

## Definition

```cpp
struct CelestialBody {
    // physics state (authoritative, double precision)
    glm::dvec3 pos_m{ 0.0 };
    glm::dvec3 vel_m{ 0.0 };
    double     mass_kg = 0.0;

    // identity
    std::string name;
    glm::vec3   color{ 1.0f };

    // visual derived state
    float       density  = 1000.0f;
    float       radius   = 1.0f;
    float       emissive = 0.0f;     // 0.0 = lit body, 1.0 = self-illuminated star
    PlanetMesh  mesh{ 3 };

    glm::vec3 renderPosition() const;
    bool      intersectsRay(const glm::vec3& rayOrigin,
                            const glm::vec3& rayDirection) const;
    void      recalculateGeometry();
    void      render(Shader& shader, bool highlight) const;
};
```

## Fields

| Field | Role | Notes |
|---|---|---|
| `pos_m` | Position in meters, world coords | Source of truth for physics |
| `vel_m` | Velocity in m/s | Tangent to current orbit |
| `mass_kg` | Mass in kg | `double` because Sun is 1.989 × 10³⁰ |
| `name` | Display string | Used in menus, hover labels |
| `color` | Diffuse base color | Linear RGB in [0,1] |
| `density` | kg/m³, drives radius | Editing recomputes `radius` |
| `radius` | World-unit display radius | Derived; do not set directly |
| `emissive` | Star bypass flag | 1.0 → fragment shader skips lighting |
| `mesh` | Owns GL state | Non-copyable, movable |

## Methods

### `glm::vec3 renderPosition() const`
Returns `pos_m / METERS_PER_WU` as a `vec3`. Used wherever world-space float coordinates are needed (model matrix, picking, camera target).

### `bool intersectsRay(rayOrigin, rayDirection) const`
Ray-sphere test against `renderPosition()` with `max(radius, 18.0f)` (the 18 WU floor lets the user click on bodies that are visually tiny). Used by `UIManager::renderPlanetPopup` for picking.

### `void recalculateGeometry()`
Recomputes `radius` from `mass_kg` and `density` (uniform-density sphere formula scaled by 10⁻⁷ to fit WU), then calls `mesh.rebuild(radius)`. Call this after mutating mass or density.

### `void render(Shader& shader, bool highlight) const`
Sets `model`, `planetColor`, and `emissive` uniforms, then issues a draw via `mesh.draw()`. The `highlight` parameter is currently unused — wiring it up to a tint or outline is a P1 task.

## Lifecycle

`CelestialBody` is a value type designed for `std::vector` storage. It contains a non-copyable `PlanetMesh`, so the struct is implicitly non-copyable and the compiler generates move ctor / move assign. That's what makes "Add Planet" work: `vector::push_back(std::move(nb))` move-constructs the body into the buffer, transferring the GL handles atomically.

The default `mesh{ 3 }` is **dormant** (no GL state allocated). You must call `recalculateGeometry()` at least once before rendering — typically at initialization or right after `push_back`.

## Edit path

When the user clicks Apply Changes in Planet Info:

```cpp
body.name     = editBuffer.name;
body.mass_kg  = static_cast<double>(editBuffer.mass);
body.density  = editBuffer.density;
body.pos_m    = glm::dvec3(editBuffer.position) * METERS_PER_WU;
body.vel_m    = glm::dvec3(editBuffer.velocity) * 1000.0;   // km/s → m/s
body.recalculateGeometry();
```

This is the same `body` reference the physics iterates over — there is no sync step. The next `physics.update()` call sees the new state.

## Why this exists

See [ADR 0001](../adr/0001-celestialbody-unified-state.md).
