#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>

class Grid
{
public:
    Grid(float size, int divisions, float height);
    ~Grid();

    void draw() const;

private:
    GLuint VAO, VBO;
    int lineCount;

    void setupGrid(float size, int divisions, float height);
};