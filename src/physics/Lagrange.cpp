#include "physics/Lagrange.h"

#include "core/Constants.h"
#include "objects/CelestialBody.h"

#include <cmath>

namespace lagrange
{

Points compute(const CelestialBody& primary, const CelestialBody& secondary)
{
    Points result;

    const glm::dvec3 separation = secondary.pos_m - primary.pos_m;
    const double R              = glm::length(separation);
    const double totalMass      = primary.mass_kg + secondary.mass_kg;

    if (R < 1.0 || totalMass <= 0.0)
        return result;

    const glm::dvec3 axis = separation / R; // unit vector primary → secondary
    const double mu       = secondary.mass_kg / totalMass;

    // Hill radius — the L1/L2 distance from the secondary along the
    // primary-secondary axis. Good to better than 1% for μ < 0.01
    // (covers every planet in this sim).
    const double hillRadius = R * std::cbrt(mu / 3.0);

    // L1: between primary and secondary, just inside the secondary.
    const glm::dvec3 L1m = secondary.pos_m - axis * hillRadius;

    // L2: beyond the secondary, opposite the primary.
    const glm::dvec3 L2m = secondary.pos_m + axis * hillRadius;

    // L3: opposite the secondary from the primary, at roughly R from the
    // primary (slightly farther because of μ).
    const glm::dvec3 L3m = primary.pos_m - axis * (R * (1.0 + (7.0 / 12.0) * mu));

    // L4, L5: equilateral-triangle points in the XZ orbital plane. Rotate
    // the separation vector by ±60° around the Y axis.
    constexpr double kCos60 = 0.5;
    const double kSin60     = std::sqrt(3.0) * 0.5;

    const glm::dvec3 L4offset(separation.x * kCos60 - separation.z * kSin60, separation.y,
                              separation.x * kSin60 + separation.z * kCos60);
    const glm::dvec3 L5offset(separation.x * kCos60 + separation.z * kSin60, separation.y,
                              -separation.x * kSin60 + separation.z * kCos60);

    const glm::dvec3 L4m = primary.pos_m + L4offset;
    const glm::dvec3 L5m = primary.pos_m + L5offset;

    result.positions[0] = glm::vec3(L1m / METERS_PER_WU);
    result.positions[1] = glm::vec3(L2m / METERS_PER_WU);
    result.positions[2] = glm::vec3(L3m / METERS_PER_WU);
    result.positions[3] = glm::vec3(L4m / METERS_PER_WU);
    result.positions[4] = glm::vec3(L5m / METERS_PER_WU);
    result.valid        = true;
    return result;
}

} // namespace lagrange
