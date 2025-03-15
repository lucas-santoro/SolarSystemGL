#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include "objects/Planet.h"

class Grid {
public:
    Grid(float size, int divisions, float height = 0.0f);
    ~Grid();

    void setupGrid(float size, int divisions, float height);
    void applyGravityDistortion(const std::vector<Planet>& planets);
    void draw() const;

private:
    GLuint VAO, VBO;
    int lineCount;
    std::vector<glm::vec3> originalPoints;
};

