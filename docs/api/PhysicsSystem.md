# `PhysicsSystem`

`src/physics/PhysicsSystem.h`

N-body Newtonian gravity, integrated with Velocity Verlet at a fixed 240 Hz internal sub-step. Owns the timestep accumulator and a pair of reusable acceleration buffers.

## Definition

```cpp
class PhysicsSystem {
public:
    float  timeScale = 864'000.0f;        // 10 days / s
    bool   paused    = false;

    static constexpr double G             = 6.67430e-11;
    static constexpr double SOFTEN        = 1e3;        // m², added to dist²
    static constexpr double FIXED_DT_REAL = 1.0 / 240.0;
    static constexpr double MAX_ACCUM     = 0.5;

    void update(std::vector<CelestialBody>& bodies, double dtReal);

private:
    double accumulator = 0.0;
    std::vector<glm::dvec3> accBuffer0;
    std::vector<glm::dvec3> accBuffer1;

    void stepOnce(std::vector<CelestialBody>& bodies, double dtReal);
    void computeAccelerations(const std::vector<CelestialBody>& bodies,
                              std::vector<glm::dvec3>& out) const;
};
```

## Public surface

| Member | Purpose |
|---|---|
| `timeScale` | Float, simulated seconds per real second. Bound to the UI logarithmic slider in `UIManager::renderMainPanel`. |
| `paused` | Bool, bound to the UI checkbox. When true, `update` returns immediately without touching the accumulator. |
| `update(bodies, dtReal)` | Called once per render frame from `main.cpp` with the frame's real `deltaTime`. |

`update` is not `const` — it owns mutable state (`accumulator`, the buffers). Both `timeScale` and `paused` are public for direct ImGui binding (`ImGui::Checkbox(&physics.paused)`).

## Algorithm

```cpp
void update(bodies, dtReal) {
    if (paused) return;
    accumulator = min(accumulator + dtReal, MAX_ACCUM);
    while (accumulator >= FIXED_DT_REAL) {
        stepOnce(bodies, FIXED_DT_REAL);
        accumulator -= FIXED_DT_REAL;
    }
}
```

`stepOnce` implements Velocity Verlet:

```cpp
double dtSim = dtReal * timeScale;
double half  = dtSim * 0.5;

computeAccelerations(bodies, accBuffer0);    // a(t)
bodies[i].vel_m += accBuffer0[i] * half;     // half-kick
bodies[i].pos_m += bodies[i].vel_m * dtSim;  // drift
computeAccelerations(bodies, accBuffer1);    // a(t+dt)
bodies[i].vel_m += accBuffer1[i] * half;     // half-kick
```

`computeAccelerations` is the standard O(N²) pairwise sum with softening:

```cpp
glm::dvec3 r     = bodies[j].pos_m - bodies[i].pos_m;
double     dist2 = dot(r, r) + SOFTEN;
double     invD  = 1.0 / sqrt(dist2);
out[i] += G * bodies[j].mass_kg * invD * invD * r * invD;
```

## Why no acceleration caching across steps?

A common optimization in Verlet is to keep `a(t+dt)` from the end of one step and use it as `a(t)` of the next — halving the cost. We skip it because the UI can mutate body positions at any time (Apply Changes), which silently invalidates any cached acceleration. Recomputing twice per step is unconditionally correct and costs ~150 pair evaluations per step at 9 bodies — negligible.

When Barnes-Hut or SoA optimizations land (P2), revisit caching with explicit invalidation.

## Adding bodies at runtime

When `UIManager::renderMainPanel` calls `bodies.push_back(...)`, the next `update` call sees a longer vector. `computeAccelerations` calls `out.assign(bodies.size(), 0.0)` which resizes the buffer to match. No special handling needed.

If you remove a body (not yet implemented), the next `assign` shrinks the buffer correctly.

## Determinism

Given identical `(bodies state, timeScale, accumulator history)`, the simulation produces bit-identical results. Frame-to-frame jitter in real `deltaTime` only affects *when* a sub-step fires, not the sub-step itself.

This matters for: reproducible bug reports, "save state and reload" features, and any future replay system.

## Why public state instead of getters/setters?

`timeScale` and `paused` are bound directly to ImGui widgets, which need raw pointers:

```cpp
ImGui::Checkbox("Paused", &physics.paused);
ImGui::SliderFloat("Time scale", &physics.timeScale, ...);
```

Wrapping in getter/setter would force a copy + writeback dance every frame. Encapsulation here would cost more than it saves.
