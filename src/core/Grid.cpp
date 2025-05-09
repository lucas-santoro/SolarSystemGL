#include <glad/glad.h>
#include "core/Grid.h"
#include <algorithm>

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
    int actualDivisions = divisions * 2;
    std::vector<GLfloat> vertices;
    float step = size / actualDivisions;
    float half = size / 2.0f;
    std::vector<glm::vec3> gridPoints;

    for (int i = 0; i <= actualDivisions; i++)
    {
        for (int j = 0; j <= actualDivisions; j++)
        {
            float x = -half + j * step;
            float z = -half + i * step;
            gridPoints.push_back(glm::vec3(x, 0.0f, z));
        }
    }

    originalPoints = gridPoints;

    for (int i = 0; i <= actualDivisions; i++)
    {
        for (int j = 0; j < actualDivisions; j++)
        {
            int index = i * (actualDivisions + 1) + j;
            vertices.push_back(gridPoints[index].x);
            vertices.push_back(gridPoints[index].y);
            vertices.push_back(gridPoints[index].z);
            vertices.push_back(gridPoints[index + 1].x);
            vertices.push_back(gridPoints[index + 1].y);
            vertices.push_back(gridPoints[index + 1].z);
        }
    }

    for (int j = 0; j <= actualDivisions; j++)
    {
        for (int i = 0; i < actualDivisions; i++)
        {
            int index = i * (actualDivisions + 1) + j;
            int nextIndex = (i + 1) * (actualDivisions + 1) + j;
            vertices.push_back(gridPoints[index].x);
            vertices.push_back(gridPoints[index].y);
            vertices.push_back(gridPoints[index].z);
            vertices.push_back(gridPoints[nextIndex].x);
            vertices.push_back(gridPoints[nextIndex].y);
            vertices.push_back(gridPoints[nextIndex].z);
        }
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

void Grid::draw(Shader& shader, const std::vector<std::shared_ptr<Planet>>& planets) const 
{
    shader.use();

    int count = std::min((int)planets.size(), 10);
    std::vector<glm::vec3> positions(count);
    std::vector<float> masses(count);

    for (int i = 0; i < count; ++i) {
        positions[i] = planets[i]->getPosition();
        masses[i] = planets[i]->getMass() / 5.97e24f; // normalize as antes
    }

    shader.setInt("planetCount", count);
    shader.setVec3Array("planetPositions", positions);
    shader.setFloatArray("planetMasses", masses);

    glBindVertexArray(VAO);
    glDrawArrays(GL_LINES, 0, lineCount);
    glBindVertexArray(0);
}
