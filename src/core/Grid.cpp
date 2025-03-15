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

void Grid::applyGravityDistortion(const std::vector<Planet>& planets)
{
    int divisions = (int)sqrt(originalPoints.size()) - 1;
    std::vector<glm::vec3> distortedPoints = originalPoints;
    const float visualScale = 0.8f;
    const float maxDistortion = 2.0f;
    const float falloffRadius = 2.0f;

    for (size_t i = 0; i < distortedPoints.size(); i++)
    {
        glm::vec3& point = distortedPoints[i];
        point.y = 0.0f;

        for (const auto& planet : planets)
        {
            glm::vec3 planetPos = planet.getPosition();
            planetPos.y = 0.0f;
            float dx = point.x - planetPos.x;
            float dz = point.z - planetPos.z;
            float distanceSquared = dx * dx + dz * dz;

            if (distanceSquared < 0.001f)
            {
                distanceSquared = 0.001f;
            }

            float massScale = planet.getMass() / 5.97e24f;
            float distortion = maxDistortion * massScale / (1.0f + distanceSquared / (falloffRadius * falloffRadius));
            point.y -= distortion * visualScale;
        }
    }

    std::vector<GLfloat> updatedVertices;

    for (int i = 0; i <= divisions; i++)
    {
        for (int j = 0; j < divisions; j++)
        {
            int index = i * (divisions + 1) + j;
            updatedVertices.push_back(distortedPoints[index].x);
            updatedVertices.push_back(distortedPoints[index].y);
            updatedVertices.push_back(distortedPoints[index].z);
            updatedVertices.push_back(distortedPoints[index + 1].x);
            updatedVertices.push_back(distortedPoints[index + 1].y);
            updatedVertices.push_back(distortedPoints[index + 1].z);
        }
    }

    for (int j = 0; j <= divisions; j++)
    {
        for (int i = 0; i < divisions; i++)
        {
            int index = i * (divisions + 1) + j;
            int nextIndex = (i + 1) * (divisions + 1) + j;
            updatedVertices.push_back(distortedPoints[index].x);
            updatedVertices.push_back(distortedPoints[index].y);
            updatedVertices.push_back(distortedPoints[index].z);
            updatedVertices.push_back(distortedPoints[nextIndex].x);
            updatedVertices.push_back(distortedPoints[nextIndex].y);
            updatedVertices.push_back(distortedPoints[nextIndex].z);
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, updatedVertices.size() * sizeof(GLfloat), updatedVertices.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Grid::draw() const
{
    glBindVertexArray(VAO);
    glDrawArrays(GL_LINES, 0, lineCount);
    glBindVertexArray(0);
}
