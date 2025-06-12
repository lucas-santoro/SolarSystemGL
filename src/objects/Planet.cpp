#define _USE_MATH_DEFINES
#include "objects/Planet.h"
#include <glad/glad.h>
#include <cmath>
#include <map>
#include <iostream>

Planet::Planet(const std::string& name, float mass, float density, glm::vec3 position, glm::vec3 velocity, glm::vec3 color, int subdivisions)
	: name(name), mass(mass), density(density), position(position), velocity(velocity), color(color), subdivisions(subdivisions)
{
    calculateRadius();
    generateIcosahedron();
    subdivide(subdivisions);
    setupMesh();
}

Planet::~Planet()
{
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}

void Planet::generateIcosahedron()
{
    float t = (1.0f + sqrt(5.0f)) / 2.0f;

    vertices = {
        glm::vec3(-1.0f,  t,  0.0f), glm::vec3(1.0f,  t,  0.0f),
        glm::vec3(-1.0f, -t,  0.0f), glm::vec3(1.0f, -t,  0.0f),
        glm::vec3(0.0f, -1.0f,  t), glm::vec3(0.0f,  1.0f,  t),
        glm::vec3(0.0f, -1.0f, -t), glm::vec3(0.0f,  1.0f, -t),
        glm::vec3(t,  0.0f, -1.0f), glm::vec3(t,  0.0f,  1.0f),
        glm::vec3(-t,  0.0f, -1.0f), glm::vec3(-t,  0.0f,  1.0f)
    };

    for (auto& v : vertices)
    {
        v = glm::normalize(v) * radius;
    }

    indices = {
        0u, 11u, 5u,  0u, 5u, 1u,  0u, 1u, 7u,  0u, 7u, 10u,  0u, 10u, 11u,
        1u, 5u, 9u,  5u, 11u, 4u,  11u, 10u, 2u,  10u, 7u, 6u,  7u, 1u, 8u,
        3u, 9u, 4u,  3u, 4u, 2u,  3u, 2u, 6u,  3u, 6u, 8u,  3u, 8u, 9u,
        4u, 9u, 5u,  2u, 4u, 11u,  6u, 2u, 10u,  8u, 6u, 7u,  9u, 8u, 1u
    };
}

void Planet::subdivide(int depth)
{
    for (int i = 0; i < depth; i++)
    {
        std::vector<unsigned int> newIndices;
        std::map<std::pair<unsigned int, unsigned int>, unsigned int> midpoints;

        auto getMidpoint = [&](unsigned int v1, unsigned int v2) -> unsigned int
            {
                std::pair<unsigned int, unsigned int> key = v1 < v2 ?
                    std::make_pair(v1, v2) : std::make_pair(v2, v1);

                if (midpoints.count(key))
                {
                    return midpoints[key];
                }

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

        indices = newIndices;
    }
}

void Planet::setupMesh()
{
    std::vector<float> vertexData;
    for (const auto& v : vertices)
    {
        vertexData.push_back(v.x);
        vertexData.push_back(v.y);
        vertexData.push_back(v.z);
    }

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

bool Planet::intersectsRay(const glm::vec3 &rayOrigin,
    const glm::vec3 &rayDirection) const
{
    float baseRadius = getRadius();
    float pickRadius = baseRadius < MIN_PICK_RADIUS
        ? MIN_PICK_RADIUS
        : baseRadius;

    glm::vec3 originToCtr = rayOrigin - position;

    float dirLenSq = glm::dot(rayDirection, rayDirection);
    float twiceProj = 2.0f * glm::dot(originToCtr, rayDirection);
    float centerDistSq = glm::dot(originToCtr, originToCtr) -
        pickRadius * pickRadius;

    float discriminant = twiceProj * twiceProj -
        4.0f * dirLenSq * centerDistSq;

    return discriminant >= 0.0f;
}



void Planet::calculateRadius()
{
    const float scaleFactor = 1e-7f;
    radius = std::cbrt((3.0f * mass) / (4.0f * M_PI * density)) * scaleFactor;
}

void Planet::recalculateGeometry() 
{
    calculateRadius();
    generateIcosahedron();
    subdivide(subdivisions);
    setupMesh();
}

void Planet::render(Shader &shader)
{
    float visualScale = 1.0f;
    if (name == "Sun") visualScale = 0.4f;

    glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
    model = glm::scale(model, glm::vec3(/*radius * */visualScale));

    shader.setMat4("model", model);
    shader.setVec3("planetColor", color);

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Planet::setPosition(const glm::vec3 &newPosition)
{ 
    position = newPosition; 
}
void Planet::setVelocity(const glm::vec3 &newVelocity)
{ 
    velocity = newVelocity;
}

void Planet::setMass(float newMass)
{
    mass = newMass;
    calculateRadius();
}

void Planet::setDensity(float newDensity) 
{
    density = newDensity;
    calculateRadius();
}

void Planet::setName(const std::string &newName) 
{
    name = newName;
}