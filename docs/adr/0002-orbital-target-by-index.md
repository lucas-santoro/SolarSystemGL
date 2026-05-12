# ADR 0002 — Camera orbital target by index, not pointer

**Status:** Accepted
**Date:** 2026-05

## Context

The previous `Camera` held its orbital target as a raw pointer to a `glm::vec3` inside a `Planet`:

```cpp
const glm::vec3* orbitalTarget = nullptr;

void setOrbitalTarget(const glm::vec3* p, float distance) {
    orbitalTarget = p;
    ...
}
```

Callers passed `selectedPlanet->getPositionPtr()`. Each frame, `getViewMatrix()` dereferenced the pointer to compute the camera position.

This worked while `planets` was `std::vector<std::shared_ptr<Planet>>` — the shared_ptr ensured the underlying `Planet` wouldn't move in memory. After the unification ([ADR 0001](0001-celestialbody-unified-state.md)), bodies live by value in `std::vector<CelestialBody>`. A `bodies.push_back(...)` can trigger reallocation, moving every element to new memory. The cached `const glm::vec3*` would dangle.

Without a fix: clicking "Add Planet" while orbital camera was active on Jupiter would crash or jump to garbage.

## Decision

Track the orbital target by **integer index** into the `bodies` vector, plus a cached position updated each frame:

```cpp
int        orbitalTargetIndex = -1;
glm::vec3  orbitalTargetPos{ 0.0f };

void setOrbitalTarget(int index, float distance);
void setOrbitalTargetPos(const glm::vec3& pos);
```

`main.cpp` refreshes the cached position once per frame, before computing the view matrix:

```cpp
int orbIdx = camera.getOrbitalTargetIndex();
if (orbIdx >= 0 && orbIdx < (int)bodies.size())
    camera.setOrbitalTargetPos(bodies[orbIdx].renderPosition());
```

The view matrix uses the cached `orbitalTargetPos`, not a pointer.

## Consequences

**Wins:**
- Indices are stable across reallocations. Adding bodies cannot dangle the target.
- The boundary between "what to look at" (index) and "where it is right now" (cached `vec3`) is explicit. Render code stays simple.
- Removing a body becomes trivially safe to design: clear the index if it equals the removed slot, decrement if it was past the removed slot. (Not implemented yet, but the design admits it.)

**Losses:**
- One extra writeback per frame from `main.cpp`. Negligible.
- If the index goes stale (e.g., a future "Remove Planet" doesn't fix up the camera), the camera silently falls back to FREE mode (the bounds check fails). That's a softer failure mode than a dangling pointer crash.

## Defense in depth

Alongside this change, `main.cpp` calls `bodies.reserve(16)` at startup. This means actual addresses inside the vector are stable for up to 16 bodies — so the original pointer-based design *would* have worked at small scale. Keeping the index approach anyway, because:

1. Defense in depth — if the reserve cap gets exceeded later, the index approach still works; the pointer approach would silently break.
2. Index is the right model conceptually — "the camera tracks body #3", not "the camera tracks the memory at this address".
