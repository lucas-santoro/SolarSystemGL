# Physics

Newtonian N-body gravity in double precision, integrated by Velocity Verlet at a fixed 240 Hz sub-step.

## Model

For each body `i`, gravitational acceleration:

```
        ___                 (r_j - r_i)
a_i =   \   G · m_j ·  ─────────────────────
        /              (|r_j - r_i|² + ε)^(3/2)
        ‾‾‾
        j≠i
```

with `ε = SOFTEN = 10³ m²` added to `dist²` as a softening term. The softening prevents the singularity at close range without affecting orbits at any realistic separation.

Constants (`src/physics/PhysicsSystem.h`):

| Symbol | Code | Value | Units |
|---|---|---|---|
| `G` | `PhysicsSystem::G` | 6.67430 × 10⁻¹¹ | m³ / (kg · s²) |
| `ε` | `PhysicsSystem::SOFTEN` | 10³ | m² |
| `dt_phys` | `PhysicsSystem::FIXED_DT_REAL` | 1/240 | s (real) |
| `cap` | `PhysicsSystem::MAX_ACCUM` | 0.5 | s (real) |

## Integrator

**Velocity Verlet** (kick-drift-kick), one sub-step:

1. `a_n = f(x_n, m)`
2. `v_{n+½} = v_n + a_n · h/2`         _(half-kick)_
3. `x_{n+1} = x_n + v_{n+½} · h`        _(drift)_
4. `a_{n+1} = f(x_{n+1}, m)`
5. `v_{n+1} = v_{n+½} + a_{n+1} · h/2`  _(half-kick)_

Where `h = dt_phys · timeScale` is the **simulated** time step. With `timeScale = 864 000` (default, 10 days/s real) and `dt_phys = 1/240`, each sub-step advances physics by 3 600 s = 1 hour of simulated time.

We don't cache `a_{n+1}` across calls because user edits via the UI can teleport positions, invalidating the cache. For N small, recomputing accelerations twice per step is cheaper than the bookkeeping.

## Fixed timestep

`PhysicsSystem::update` is called once per frame with the frame's real `deltaTime`. We accumulate that into a buffer and consume in fixed `1/240 s` chunks:

```cpp
accumulator = std::min(accumulator + dtReal, MAX_ACCUM);
while (accumulator >= FIXED_DT_REAL) {
    stepOnce(bodies, FIXED_DT_REAL);
    accumulator -= FIXED_DT_REAL;
}
```

The `MAX_ACCUM` cap prevents the *spiral of death*: if the frame is suspended for many seconds (debugger, OS hibernation), the accumulator can't grow unboundedly. Anything older than half a second is dropped.

The simulation is **deterministic** for a given starting state, `timeScale`, and accumulator history. Frame-to-frame jitter in real `deltaTime` does not affect physics output.

## Time scale

`PhysicsSystem::timeScale` is a `float` exposed to the UI via a logarithmic slider (`UIManager::renderMainPanel`) ranging from `1.0` to `5 × 10⁶`. Each sub-step uses the current value, so changing the slider is immediate.

| Setting | Real second simulates | Mercury orbital period (~88 days) | Visual cadence |
|---|---|---|---|
| `1.0` | 1 s | 88 × 24 × 3600 / 1 ≈ 90 years (real time) | Almost frozen |
| `864 000` | 10 days | ~8.8 s | Comfortable |
| `5 × 10⁶` | ~58 days | ~1.5 s | Fast |

## Conservation properties

Velocity Verlet is a *symplectic* integrator: energy oscillates within a bounded envelope but does not drift secularly. This is the main reason we chose it over plain Euler-Cromer (which has bounded drift but worse phase-space behavior).

The barycenter (center of mass) does **drift** because the default initial conditions assign zero velocity to the Sun and tangential velocities to the planets. The total momentum is non-zero, so the system as a whole moves. A `P1` item tracks the fix: subtract the total-momentum drift from all bodies at startup.

## Real planet data

Position, mass, density, and orbital velocity come from `src/main.cpp`'s `Init` table. Sources:

| Body | Mass (kg) | Density (kg/m³) | Distance (AU) | v (m/s) |
|---|---|---|---|---|
| Sun | 1.989 × 10³⁰ | 1 408 | 0 | 0 |
| Mercury | 3.301 × 10²³ | 5 427 | 0.387 | 47 900 |
| Venus | 4.868 × 10²⁴ | 5 243 | 0.723 | 35 000 |
| Earth | 5.972 × 10²⁴ | 5 514 | 1.000 | 29 780 |
| Mars | 6.417 × 10²³ | 3 933 | 1.524 | 24 100 |
| Jupiter | 1.898 × 10²⁷ | 1 326 | 5.203 | 13 070 |
| Saturn | 5.683 × 10²⁶ | 687 | 9.537 | 9 680 |
| Uranus | 8.681 × 10²⁵ | 1 271 | 19.191 | 6 800 |
| Neptune | 1.024 × 10²⁶ | 1 638 | 30.070 | 5 430 |

All initial velocities are along +z (so planets orbit counterclockwise when viewed from +y).
