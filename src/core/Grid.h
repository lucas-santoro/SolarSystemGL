#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include "objects/CelestialBody.h"

class Grid
{
public:
    Grid(float size, int divisions, float height = 0.0f);
    ~Grid();

    Grid(const Grid&)            = delete;
    Grid& operator=(const Grid&) = delete;
    Grid(Grid&& other) noexcept;
    Grid& operator=(Grid&& other) noexcept;

    void setupGrid(float size, int divisions, float height);
    void draw(Shader& shader, const std::vector<CelestialBody>& bodies) const;

private:
    GLuint VAO = 0;
    GLuint VBO = 0;
    int    lineCount = 0;
};
