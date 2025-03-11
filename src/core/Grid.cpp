#include <glad/glad.h>
#include "core/Grid.h"


Grid::Grid(float size, int divisions, float height)
{
    setupGrid(size, divisions, height);
}

Grid::~Grid()
{
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}

void Grid::setupGrid(float size, int divisions, float height)
{
    std::vector<GLfloat> vertices;
    float step = size / divisions;
    float half = size / 2.0f;

    for (int i = 0; i <= divisions; i++)
    {
        float pos = -half + i * step;

        vertices.push_back(pos);
        vertices.push_back(height);
        vertices.push_back(-half);

        vertices.push_back(pos);
        vertices.push_back(height);
        vertices.push_back(half);

        vertices.push_back(-half);
        vertices.push_back(height);
        vertices.push_back(pos);

        vertices.push_back(half);
        vertices.push_back(height);
        vertices.push_back(pos);
    }

    lineCount = (int)vertices.size() / 3;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Grid::draw() const
{
    glBindVertexArray(VAO);
    glDrawArrays(GL_LINES, 0, lineCount);
    glBindVertexArray(0);
}
