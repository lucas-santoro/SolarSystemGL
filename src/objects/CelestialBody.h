#pragma once

#include <glm/glm.hpp>
#include <string>
#include <deque>
#include "objects/PlanetMesh.h"
#include "core/Shader.h"

/**
 * @file CelestialBody.h
 * @brief One simulated body: physics state + visual identity + render mesh.
 */

/**
 * @brief Auto-derived classification used by the UI to label bodies in the
 *        System dropdown and the Planet Info header.
 *
 * Mass and emissive are the only inputs — no orbit-tree analysis. Moons end
 * up tagged as Planet under this scheme, which is acceptable since the
 * default presets don't include moons.
 */
enum class BodyType
{
    Star,      ///< emissive >= 0.5
    Planet,    ///< mass >= 1e22 kg, non-emissive
    Asteroid   ///< mass <  1e22 kg, non-emissive
};

/**
 * @brief A single celestial body — the canonical owner of one entity's state.
 *
 * Kept as a `struct` with public fields by intent: this is a *data carrier*
 * for the physics, render, and UI layers, all of which read and write the
 * same memory. The behavior methods (#renderPosition, #intersectsRay,
 * #focusDistance, #recalculateGeometry, #render) provide encapsulated
 * operations on top of that data.
 *
 * Implicitly non-copyable because it contains a non-copyable PlanetMesh, and
 * therefore implicitly movable — which is exactly what `std::vector` needs
 * for "Add Planet" / reallocation to work without leaking GL handles.
 */
struct CelestialBody
{
    /** @brief Position in meters (world coordinates). Source of truth for physics. */
    glm::dvec3 pos_m{ 0.0 };

    /** @brief Velocity in meters per second. */
    glm::dvec3 vel_m{ 0.0 };

    /** @brief Mass in kilograms. `double` because the Sun is ~2 × 10^30 kg. */
    double mass_kg = 0.0;

    /** @brief Display name used in menus, hover labels, and Planet Info. */
    std::string name;

    /** @brief Diffuse base color in linear RGB, components in `[0, 1]`. */
    glm::vec3 color{ 1.0f };

    /** @brief Density in kg/m^3 — drives @ref radius via #recalculateGeometry. */
    float density = 1000.0f;

    /** @brief World-unit display radius, derived from mass + density. Do not set directly. */
    float radius = 1.0f;

    /** @brief 1.0 = star (skips lighting, gets a halo); 0.0 = lit body. */
    float emissive = 0.0f;

    /** @brief Visual-only model-matrix multiplier (e.g. Sun = 0.4 so it doesn't engulf orbits). */
    float displayScale = 1.0f;

    /** @brief When `true`, a procedural ring disk is rendered around this body. */
    bool hasRings = false;

    /** @brief When `true`, an extra translucent atmosphere shell is rendered around this body. */
    bool hasAtmosphere = false;

    /** @brief Atmosphere shell thickness as a fraction of the body radius (e.g. 0.05 = +5%). */
    float atmosphereHeight = 0.05f;

    /** @brief Linear-RGB tint of the atmosphere shell (Rayleigh-flavored, e.g. blue for Earth). */
    glm::vec3 atmosphereColor{ 0.40f, 0.60f, 1.00f };

    /** @brief Multiplier applied to the atmosphere shell's overall intensity. */
    float atmosphereDensity = 1.0f;

    /** @brief GL state for this body's subdivided icosahedron mesh. */
    PlanetMesh mesh{ 3 };

    /** @brief Orbit history — last N render-space positions, oldest at front. */
    std::deque<glm::vec3> trailPoints;

    /** @return The body's position in world units (meters / METERS_PER_WU). */
    glm::vec3 renderPosition() const;

    /**
     * @brief Ray-sphere intersection test for picking.
     * @param rayOrigin    Origin of the ray in world units.
     * @param rayDirection Normalized ray direction.
     * @return             `true` if the ray hits a 18-WU-minimum bounding sphere centered on the body.
     */
    bool intersectsRay(const glm::vec3& rayOrigin, const glm::vec3& rayDirection) const;

    /**
     * @brief Recommended orbit-camera distance from this body.
     *
     * Tuned so even tiny inner planets are framed comfortably without zooming
     * the camera inside larger bodies.
     */
    float focusDistance() const;

    /** @brief Recompute @ref radius from mass + density, then rebuild the GPU mesh. */
    void recalculateGeometry();

    /**
     * @brief Issue the draw call for this body.
     * @param shader Bound body shader — receives `model`, `planetColor`, `emissive`.
     */
    void render(Shader& shader) const;

    /// @return The auto-derived BodyType from this body's mass + emissive.
    BodyType classify() const;
};

/// @return Short display label for the chip ("Star" / "Planet" / "Asteroid").
const char* bodyTypeLabel(BodyType type);

/// @return Tint used to colour the chip in the System dropdown / Planet Info.
glm::vec3   bodyTypeColor(BodyType type);
