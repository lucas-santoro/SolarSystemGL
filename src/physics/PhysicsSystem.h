#pragma once
#include <vector>
#include <glm/glm.hpp>
#include "objects/CelestialBody.h"

class PhysicsSystem
{
public:
    float  timeScale = 864'000.0f; // 10 days / s exactly (10 * 86400 s/day)
    bool   paused    = false;

    static constexpr double G             = 6.67430e-11;
    static constexpr double SOFTEN        = 1e3;          // m^2, added to dist^2
    static constexpr double FIXED_DT_REAL = 1.0 / 240.0;  // 4.16 ms physics step
    static constexpr double MAX_ACCUM     = 0.5;          // spiral-of-death cap

    void update(std::vector<CelestialBody>& bodies, double dtReal);

private:
    double accumulator = 0.0;
    std::vector<glm::dvec3> accBuffer0;
    std::vector<glm::dvec3> accBuffer1;

    void stepOnce(std::vector<CelestialBody>& bodies, double dtReal);
    void computeAccelerations(const std::vector<CelestialBody>& bodies,
                              std::vector<glm::dvec3>& out) const;
};
