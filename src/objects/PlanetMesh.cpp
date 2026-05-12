#include "objects/PlanetMesh.h"
#include <cmath>
#include <map>
#include <utility>

PlanetMesh::PlanetMesh(int subdivisions)
    : subdivisions(subdivisions)
{
}

PlanetMesh::~PlanetMesh()
{
    releaseGL();
}

PlanetMesh::PlanetMesh(PlanetMesh&& other) noexcept
    : VAO(other.VAO), VBO(other.VBO), EBO(other.EBO),
      vertices(std::move(other.vertices)),
      indices(std::move(other.indices)),
      subdivisions(other.subdivisions)
{
    other.VAO = 0;
    other.VBO = 0;
    other.EBO = 0;
}

PlanetMesh& PlanetMesh::operator=(PlanetMesh&& other) noexcept
{
    if (this != &other)
    {
        releaseGL();
        VAO = other.VAO;
        VBO = other.VBO;
        EBO = other.EBO;
        vertices     = std::move(other.vertices);
        indices      = std::move(other.indices);
        subdivisions = other.subdivisions;
        other.VAO = 0;
        other.VBO = 0;
        other.EBO = 0;
    }
    return *this;
}

void PlanetMesh::releaseGL()
{
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (EBO) glDeleteBuffers(1, &EBO);
    VAO = VBO = EBO = 0;
}

void PlanetMesh::rebuild(float radius)
{
    const float t = (1.0f + std::sqrt(5.0f)) / 2.0f;

    vertices = {
        glm::vec3(-1.0f,  t,  0.0f), glm::vec3(1.0f,  t,  0.0f),
        glm::vec3(-1.0f, -t,  0.0f), glm::vec3(1.0f, -t,  0.0f),
        glm::vec3( 0.0f, -1.0f,  t), glm::vec3(0.0f,  1.0f,  t),
        glm::vec3( 0.0f, -1.0f, -t), glm::vec3(0.0f,  1.0f, -t),
        glm::vec3( t,  0.0f, -1.0f), glm::vec3(t,  0.0f,  1.0f),
        glm::vec3(-t,  0.0f, -1.0f), glm::vec3(-t,  0.0f,  1.0f)
    };
    for (auto& v : vertices)
        v = glm::normalize(v) * radius;

    indices = {
        0u, 11u, 5u,  0u, 5u, 1u,  0u, 1u, 7u,  0u, 7u, 10u,  0u, 10u, 11u,
        1u, 5u, 9u,  5u, 11u, 4u,  11u, 10u, 2u,  10u, 7u, 6u,  7u, 1u, 8u,
        3u, 9u, 4u,  3u, 4u, 2u,  3u, 2u, 6u,  3u, 6u, 8u,  3u, 8u, 9u,
        4u, 9u, 5u,  2u, 4u, 11u, 6u, 2u, 10u,  8u, 6u, 7u,  9u, 8u, 1u
    };

    for (int i = 0; i < subdivisions; ++i)
    {
        std::vector<unsigned int> newIndices;
        std::map<std::pair<unsigned int, unsigned int>, unsigned int> midpoints;

        auto getMidpoint = [&](unsigned int v1, unsigned int v2) -> unsigned int
        {
            auto key = v1 < v2 ? std::make_pair(v1, v2) : std::make_pair(v2, v1);
            auto it  = midpoints.find(key);
            if (it != midpoints.end())
                return it->second;

            glm::vec3 mid = glm::normalize((vertices[v1] + vertices[v2]) * 0.5f) * radius;
            vertices.push_back(mid);
            unsigned int index = static_cast<unsigned int>(vertices.size() - 1);
            midpoints[key] = index;
            return index;
        };

        for (size_t j = 0; j < indices.size(); j += 3)
        {
            unsigned int v1 = indices[j];
            unsigned int v2 = indices[j + 1];
            unsigned int v3 = indices[j + 2];

            unsigned int a = getMidpoint(v1, v2);
            unsigned int b = getMidpoint(v2, v3);
            unsigned int c = getMidpoint(v3, v1);

            newIndices.insert(newIndices.end(), { v1, a, c, v2, b, a, v3, c, b, a, b, c });
        }
        indices = std::move(newIndices);
    }

    if (!VAO) glGenVertexArrays(1, &VAO);
    if (!VBO) glGenBuffers(1, &VBO);
    if (!EBO) glGenBuffers(1, &EBO);

    std::vector<float> vertexData;
    vertexData.reserve(vertices.size() * 3);
    for (const auto& v : vertices)
    {
        vertexData.push_back(v.x);
        vertexData.push_back(v.y);
        vertexData.push_back(v.z);
    }

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER,
                 vertexData.size() * sizeof(float),
                 vertexData.data(),
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 indices.size() * sizeof(unsigned int),
                 indices.data(),
                 GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void PlanetMesh::draw() const
{
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES,
                   static_cast<GLsizei>(indices.size()),
                   GL_UNSIGNED_INT,
                   nullptr);
    glBindVertexArray(0);
}
