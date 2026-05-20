#pragma once

#include <glm/glm.hpp>

#include <array>

struct CelestialBody;

/**
 * @file Lagrange.h
 * @brief Restricted three-body Lagrange-point computation.
 *
 * Computes the five Lagrange points of a (primary, secondary) pair from the
 * standard restricted-three-body formulae:
 * - L1, L2: Hill-radius approximation `r ≈ R · cbrt(μ/3)` along the
 *   primary-secondary axis.
 * - L3: leading-order asymptote `r ≈ R · (1 + 7μ/12)` on the far side of
 *   the primary.
 * - L4, L5: equilateral-triangle construction in the orbital plane (60°
 *   ahead / behind the secondary, taken to be the XZ plane).
 */

namespace lagrange {

/// Five Lagrange-point positions in world units, plus a validity flag.
struct Points
{
    std::array<glm::vec3, 5> positions{};  ///< L1, L2, L3, L4, L5
    bool                     valid = false;
};

/**
 * @brief Compute the five Lagrange points between @p primary and @p secondary.
 *
 * The orbital plane is assumed to be XZ (the convention used everywhere
 * else in the renderer). Returns `valid == false` if the two bodies share
 * a position or have zero combined mass.
 */
Points compute(const CelestialBody& primary, const CelestialBody& secondary);

}  // namespace lagrange
