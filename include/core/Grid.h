#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include "objects/Planet.h"
#include <memory>

class Grid {
public:
    Grid(float size, int divisions, float height = 0.0f);
    ~Grid();

    void setupGrid(float size, int divisions, float height);
    void applyGravityDistortion(std::vector<std::shared_ptr<Planet>>& planets);
    void draw() const;

private:
    GLuint VAO, VBO;
    int lineCount;
    std::vector<glm::vec3> originalPoints;
};

