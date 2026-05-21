#define _USE_MATH_DEFINES
#include "objects/CelestialBody.h"
#include "core/Constants.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <algorithm>

static constexpr float MIN_PICK_RADIUS = 18.0f;

glm::vec3 CelestialBody::renderPosition() const
{
    return glm::vec3(pos_m / METERS_PER_WU);
}

bool CelestialBody::intersectsRay(const glm::vec3& rayOrigin,
                                  const glm::vec3& rayDirection) const
{
    float pickRadius = std::max(radius, MIN_PICK_RADIUS);
    glm::vec3 worldPos = renderPosition();
    glm::vec3 originToCtr = rayOrigin - worldPos;

    float dirLenSq     = glm::dot(rayDirection, rayDirection);
    float twiceProj    = 2.0f * glm::dot(originToCtr, rayDirection);
    float centerDistSq = glm::dot(originToCtr, originToCtr) - pickRadius * pickRadius;
    float discriminant = twiceProj * twiceProj - 4.0f * dirLenSq * centerDistSq;
    return discriminant >= 0.0f;
}

float CelestialBody::focusDistance() const
{
    // Tuned so even tiny inner planets are framed comfortably without
    // zooming the camera inside larger bodies.
    constexpr float kMinimumFocusDistance = 50.0f;
    constexpr float kBodyRadiusMultiplier = 4.0f;
    return std::max(kMinimumFocusDistance, radius * displayScale * kBodyRadiusMultiplier);
}

void CelestialBody::recalculateGeometry()
{
    constexpr float scaleFactor = 1e-7f;
    const float massF = static_cast<float>(mass_kg);
    radius = std::cbrt((3.0f * massF) / (4.0f * static_cast<float>(M_PI) * density)) * scaleFactor;
    mesh.rebuild(radius);
}

void CelestialBody::render(Shader& shader) const
{
#ifndef SOLARSYSTEM_BUILD_WEB
    // glPolygonMode doesn't exist in WebGL2 / OpenGL ES, and the default
    // is GL_FILL on every conformant context anyway — safe to skip.
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif

    glm::mat4 model = glm::translate(glm::mat4(1.0f), renderPosition());
    model = glm::scale(model, glm::vec3(displayScale));
    shader.setMat4("model", model);
    shader.setVec3("planetColor", color);
    shader.setFloat("emissive", emissive);

    mesh.draw();
}

BodyType CelestialBody::classify() const
{
    if (emissive >= 0.5f)         return BodyType::Star;
    if (mass_kg  >= 1.0e22)       return BodyType::Planet;
    return BodyType::Asteroid;
}

const char* bodyTypeLabel(BodyType type)
{
    switch (type)
    {
        case BodyType::Star:     return "Star";
        case BodyType::Planet:   return "Planet";
        case BodyType::Asteroid: return "Asteroid";
    }
    return "?";
}

glm::vec3 bodyTypeColor(BodyType type)
{
    switch (type)
    {
        case BodyType::Star:     return glm::vec3(1.00f, 0.90f, 0.30f);  // warm yellow
        case BodyType::Planet:   return glm::vec3(0.40f, 0.75f, 1.00f);  // cyan-blue
        case BodyType::Asteroid: return glm::vec3(0.70f, 0.70f, 0.70f);  // neutral grey
    }
    return glm::vec3(1.0f);
}
