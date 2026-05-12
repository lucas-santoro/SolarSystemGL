#pragma once
#include <glm/glm.hpp>
#include <string>
#include <deque>
#include "objects/PlanetMesh.h"
#include "core/Shader.h"

struct CelestialBody
{
    // physics state (authoritative, double precision)
    glm::dvec3 pos_m{ 0.0 };
    glm::dvec3 vel_m{ 0.0 };
    double     mass_kg = 0.0;

    // identity
    std::string name;
    glm::vec3   color{ 1.0f };

    // visual derived state
    float       density      = 1000.0f;
    float       radius       = 1.0f;
    float       emissive     = 0.0f;   // 0.0 = lit body, 1.0 = self-illuminated star
    float       displayScale = 1.0f;   // visual-only model-matrix multiplier; e.g. Sun = 0.4
    bool        hasRings     = false;  // procedural ring disk rendered around this body
    PlanetMesh  mesh{ 3 };

    // orbit trail — last N rendered positions, oldest at front
    std::deque<glm::vec3> trailPoints;

    // queries
    glm::vec3 renderPosition() const;
    bool      intersectsRay(const glm::vec3& rayOrigin,
                            const glm::vec3& rayDirection) const;

    // mutation
    void recalculateGeometry();
    void render(Shader& shader, bool highlight) const;
};
