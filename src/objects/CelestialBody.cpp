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

void CelestialBody::recalculateGeometry()
{
    constexpr float scaleFactor = 1e-7f;
    const float massF = static_cast<float>(mass_kg);
    radius = std::cbrt((3.0f * massF) / (4.0f * static_cast<float>(M_PI) * density)) * scaleFactor;
    mesh.rebuild(radius);
}

void CelestialBody::render(Shader& shader, bool highlight) const
{
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    const float hoverScale = highlight ? 1.10f : 1.0f;

    glm::mat4 model = glm::translate(glm::mat4(1.0f), renderPosition());
    model = glm::scale(model, glm::vec3(displayScale * hoverScale));
    shader.setMat4("model", model);
    shader.setVec3("planetColor", color);
    shader.setFloat("emissive", emissive);

    mesh.draw();
}
