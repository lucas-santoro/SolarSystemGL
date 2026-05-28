# ADR 0001 — Unify `Planet` and `BodyState` into `CelestialBody`

**Status:** Accepted
**Date:** 2026-05

## Context

The pre-unification codebase carried two parallel vectors of bodies:

- `std::vector<std::shared_ptr<Planet>>` — visual state: `glm::vec3` position (float, in WU), color, mesh.
- `std::vector<BodyState>` — physics state: `glm::dvec3` position (double, in meters), velocity, mass.

They were kept in sync by a one-way loop in `main.cpp`:

```cpp
for (size_t i = 0; i < planets.size(); ++i) {
    glm::vec3 renderPos = glm::vec3(bodies[i].pos_m / METERS_PER_WU);
    planets[i]->setPosition(renderPos);
}
```

The UI edit panel (`UIManager::renderPlanetInfo`) wrote edits to `Planet` only. The next physics step read from `BodyState`, ignoring edits. The frame after, the sync loop overwrote `Planet::position` again with stale physics output. End result: Apply Changes did nothing.

The same bug appeared in "Add Planet" — new bodies entered `planets` but not `bodies`, so they never gravitated.

## Decision

Merge the two structures into one:

```cpp
struct CelestialBody {
    glm::dvec3 pos_m, vel_m;
    double     mass_kg;
    std::string name;
    glm::vec3   color;
    float       density, radius, emissive;
    PlanetMesh  mesh;
    ...
};

std::vector<CelestialBody> bodies;  // single source of truth
```

Physics, UI, render, and picking all operate on the same vector. The sync loop is deleted.

GL resources (`VAO`, `VBO`, `EBO`) are pulled out into a separate `PlanetMesh` class with explicit Rule of 5, so `CelestialBody` becomes a movable value type suitable for `std::vector` storage.

## Consequences

**Wins:**
- The bug class that produced "Apply does nothing" is no longer representable.
- "Add Planet" works correctly: pushing into the one vector means the new body gravitates immediately.
- Reduced state coupling — no more "remember to update both sides".
- `std::shared_ptr` overhead eliminated; bodies live by value in the vector.

**Losses:**
- `CelestialBody` mixes concerns (physics + identity + visual). A purist might prefer ECS-style separation. The pragmatic tradeoff is acceptable at 9–50 bodies; revisit if entity counts grow.
- `PlanetMesh` requires explicit move ctor / assign — Rule of 5 boilerplate. This forces the user (or anyone who composes `CelestialBody` into a vector) to think about value semantics, which is mostly a good thing.

**Spillover bugs fixed:**
- Camera's orbital target was a `const glm::vec3*` pointing into `Planet::position`. A `bodies.push_back(...)` reallocation would invalidate it. Fixing the unification revealed this as a separate concern — addressed in [ADR 0002](0002-orbital-target-by-index.md).

## Considered alternatives

- **Keep both structures, add bidirectional sync** — patch the immediate bug without addressing the root. Would re-emerge in any future feature that touched body state.
- **Make `Planet` a view over `BodyState`** — keeps the two-class structure but with a single source of truth. Cleaner than the patch, but adds reference / pointer indirection and brings the same lifetime risks as the existing pointer-based Camera target.

The chosen unification is the simplest design that makes the bug impossible.
