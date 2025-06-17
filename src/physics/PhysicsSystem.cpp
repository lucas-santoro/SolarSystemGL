#include "PhysicsSystem.h"
#include <glm/glm.hpp>

void PhysicsSystem::update(std::vector<BodyState>& bodies, double dtReal) const
{
    double dtSim = dtReal * timeScale;

    for (size_t i = 0; i < bodies.size(); ++i)
    {
        glm::dvec3 acc(0.0);

        for (size_t j = 0; j < bodies.size(); ++j)
        {
            if (i == j) continue;

            glm::dvec3 r = bodies[j].pos_m - bodies[i].pos_m;
            double     dist2 = glm::dot(r, r) + SOFTEN;
            double     invD = 1.0 / sqrt(dist2);

            acc += (G * bodies[j].mass_kg * invD * invD) * r * invD;
        }

        bodies[i].vel_m += acc * dtSim;
    }

    for (auto& b : bodies)
        b.pos_m += b.vel_m * dtSim;
}
