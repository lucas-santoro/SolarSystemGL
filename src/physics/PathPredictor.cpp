#include "PathPredictor.h"

#include "core/Constants.h"
#include "objects/CelestialBody.h"
#include "physics/PhysicsSystem.h"

#include <cmath>

void PathPredictor::computeAccelerations(std::vector<glm::dvec3>& out) const
{
    out.assign(shadowBodies_.size(), glm::dvec3(0.0));

    for (size_t i = 0; i < shadowBodies_.size(); ++i)
    {
        for (size_t j = 0; j < shadowBodies_.size(); ++j)
        {
            if (i == j)
                continue;

            const glm::dvec3 r = shadowBodies_[j].pos_m - shadowBodies_[i].pos_m;
            const double dist2 = glm::dot(r, r) + PhysicsSystem::SOFTEN;
            const double invD  = 1.0 / std::sqrt(dist2);

            out[i] += (PhysicsSystem::G * shadowBodies_[j].mass_kg * invD * invD) * r * invD;
        }
    }
}

void PathPredictor::stepOnce(double dtSim)
{
    const double halfDt = dtSim * 0.5;

    computeAccelerations(accBuffer0_);
    for (size_t i = 0; i < shadowBodies_.size(); ++i)
        shadowBodies_[i].vel_m += accBuffer0_[i] * halfDt;

    for (auto& body : shadowBodies_)
        body.pos_m += body.vel_m * dtSim;

    computeAccelerations(accBuffer1_);
    for (size_t i = 0; i < shadowBodies_.size(); ++i)
        shadowBodies_[i].vel_m += accBuffer1_[i] * halfDt;
}

double PathPredictor::estimateOrbitalPeriod(const std::vector<CelestialBody>& bodies, int selectedIdx)
{
    if (selectedIdx < 0 || selectedIdx >= static_cast<int>(bodies.size()))
        return 0.0;

    int centralIdx   = -1;
    double maxMassKg = 0.0;
    for (size_t i = 0; i < bodies.size(); ++i)
    {
        if (static_cast<int>(i) == selectedIdx)
            continue;
        if (bodies[i].mass_kg > maxMassKg)
        {
            maxMassKg  = bodies[i].mass_kg;
            centralIdx = static_cast<int>(i);
        }
    }

    if (centralIdx < 0 || maxMassKg <= 0.0)
        return 0.0;

    const glm::dvec3 separation = bodies[selectedIdx].pos_m - bodies[centralIdx].pos_m;
    const double radius         = glm::length(separation);
    if (radius <= 0.0)
        return 0.0;

    constexpr double kTwoPi = 6.283185307179586;
    return kTwoPi * std::sqrt(radius * radius * radius / (PhysicsSystem::G * maxMassKg));
}

void PathPredictor::recompute(const std::vector<CelestialBody>& bodies, int selectedIdx,
                              double horizonSeconds, int sampleCount)
{
    path_.clear();

    if (selectedIdx < 0 || selectedIdx >= static_cast<int>(bodies.size()))
        return;
    if (sampleCount <= 0 || horizonSeconds <= 0.0)
        return;

    shadowBodies_.resize(bodies.size());
    for (size_t i = 0; i < bodies.size(); ++i)
    {
        shadowBodies_[i].pos_m   = bodies[i].pos_m;
        shadowBodies_[i].vel_m   = bodies[i].vel_m;
        shadowBodies_[i].mass_kg = bodies[i].mass_kg;
    }

    path_.reserve(static_cast<size_t>(sampleCount));
    const double dtSim = horizonSeconds / static_cast<double>(sampleCount);

    for (int step = 0; step < sampleCount; ++step)
    {
        stepOnce(dtSim);
        const glm::dvec3 worldUnitPos = shadowBodies_[selectedIdx].pos_m / METERS_PER_WU;
        path_.push_back(glm::vec3(worldUnitPos));
    }
}
