#pragma once
#include <glm/glm.hpp>

struct BodyState
{
    glm::dvec3 pos_m;
    glm::dvec3 vel_m;
    double      mass_kg;
};
