#include <glad/glad.h>
#include "core/Grid.h"
#include <algorithm>
#include <utility>

Grid::Grid(float size, int divisions, float height)
{
    setupGrid(size, divisions, height);
}

Grid::~Grid()
{
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
}

Grid::Grid(Grid&& other) noexcept
    : VAO(other.VAO), VBO(other.VBO), lineCount(other.lineCount)
{
    other.VAO = 0;
    other.VBO = 0;
}

Grid& Grid::operator=(Grid&& other) noexcept
{
    if (this != &other)
    {
        if (VAO) glDeleteVertexArrays(1, &VAO);
        if (VBO) glDeleteBuffers(1, &VBO);
        VAO = other.VAO;
        VBO = other.VBO;
        lineCount = other.lineCount;
        other.VAO = 0;
        other.VBO = 0;
    }
    return *this;
}

void Grid::setupGrid(float size, int divisions, float /*height*/)
{
    const int actualDivisions = divisions * 2;
    const float step = size / actualDivisions;
    const float half = size / 2.0f;

    std::vector<glm::vec3> gridPoints;
    gridPoints.reserve(static_cast<size_t>(actualDivisions + 1) * (actualDivisions + 1));
    for (int i = 0; i <= actualDivisions; i++)
    {
        for (int j = 0; j <= actualDivisions; j++)
        {
            const float x = -half + j * step;
            const float z = -half + i * step;
            gridPoints.emplace_back(x, 0.0f, z);
        }
    }

    std::vector<GLfloat> vertices;
    vertices.reserve(gridPoints.size() * 6);

    // horizontal lines
    for (int i = 0; i <= actualDivisions; i++)
    {
        for (int j = 0; j < actualDivisions; j++)
        {
            const int idx = i * (actualDivisions + 1) + j;
            const auto& a = gridPoints[idx];
            const auto& b = gridPoints[idx + 1];
            vertices.insert(vertices.end(), { a.x, a.y, a.z, b.x, b.y, b.z });
        }
    }

    // vertical lines
    for (int j = 0; j <= actualDivisions; j++)
    {
        for (int i = 0; i < actualDivisions; i++)
        {
            const int idx     = i       * (actualDivisions + 1) + j;
            const int nextIdx = (i + 1) * (actualDivisions + 1) + j;
            const auto& a = gridPoints[idx];
            const auto& b = gridPoints[nextIdx];
            vertices.insert(vertices.end(), { a.x, a.y, a.z, b.x, b.y, b.z });
        }
    }

    lineCount = static_cast<int>(vertices.size()) / 3;

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

void Grid::draw(Shader& shader, const std::vector<CelestialBody>& bodies) const
{
    shader.use();

    const int count = std::min(static_cast<int>(bodies.size()), 10);
    std::vector<glm::vec3> positions(count);
    std::vector<float>     masses(count);

    for (int i = 0; i < count; ++i)
    {
        positions[i] = bodies[i].renderPosition();
        masses[i]    = static_cast<float>(bodies[i].mass_kg / 5.97e24);
    }

    shader.setInt("planetCount", count);
    shader.setVec3Array("planetPositions", positions);
    shader.setFloatArray("planetMasses", masses);

    glBindVertexArray(VAO);
    glDrawArrays(GL_LINES, 0, lineCount);
    glBindVertexArray(0);
}
