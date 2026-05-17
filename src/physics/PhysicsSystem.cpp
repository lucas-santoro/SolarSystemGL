#include "physics/PhysicsSystem.h"
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>

void PhysicsSystem::computeAccelerations(const std::vector<CelestialBody>& bodies,
                                         std::vector<glm::dvec3>& out) const
{
    out.assign(bodies.size(), glm::dvec3(0.0));

    for (size_t i = 0; i < bodies.size(); ++i)
    {
        for (size_t j = 0; j < bodies.size(); ++j)
        {
            if (i == j) continue;

            glm::dvec3 r     = bodies[j].pos_m - bodies[i].pos_m;
            double     dist2 = glm::dot(r, r) + SOFTEN;
            double     invD  = 1.0 / std::sqrt(dist2);

            out[i] += (G * bodies[j].mass_kg * invD * invD) * r * invD;
        }
    }
}

void PhysicsSystem::stepOnce(std::vector<CelestialBody>& bodies, double dtReal)
{
    const double dtSim  = dtReal * static_cast<double>(timeScale);
    const double halfDt = dtSim * 0.5;

    // Velocity Verlet (kick-drift-kick): half-kick with a(t), drift, recompute a(t+h), half-kick.
    // No acceleration caching across calls — Apply Changes can teleport a body and invalidate
    // any cached accelerations. For N small, recomputing twice per step is cheaper than the
    // bookkeeping to invalidate.
    computeAccelerations(bodies, accBuffer0);

    for (size_t i = 0; i < bodies.size(); ++i)
        bodies[i].vel_m += accBuffer0[i] * halfDt;

    for (auto& b : bodies)
        b.pos_m += b.vel_m * dtSim;

    computeAccelerations(bodies, accBuffer1);

    for (size_t i = 0; i < bodies.size(); ++i)
        bodies[i].vel_m += accBuffer1[i] * halfDt;
}

void PhysicsSystem::update(std::vector<CelestialBody>& bodies, double dtReal)
{
    if (paused) return;

    accumulator = std::min(accumulator + dtReal, MAX_ACCUM);

    while (accumulator >= FIXED_DT_REAL)
    {
        stepOnce(bodies, FIXED_DT_REAL);
        simulatedTime_ += FIXED_DT_REAL * static_cast<double>(timeScale);
        accumulator    -= FIXED_DT_REAL;
    }
}

void PhysicsSystem::reset()
{
    accumulator    = 0.0;
    simulatedTime_ = 0.0;
    accBuffer0.clear();
    accBuffer1.clear();
}
