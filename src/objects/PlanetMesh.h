#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

class PlanetMesh
{
public:
    explicit PlanetMesh(int subdivisions = 3);
    ~PlanetMesh();

    PlanetMesh(const PlanetMesh&) = delete;
    PlanetMesh& operator=(const PlanetMesh&) = delete;
    PlanetMesh(PlanetMesh&& other) noexcept;
    PlanetMesh& operator=(PlanetMesh&& other) noexcept;

    void rebuild(float radius);
    void draw() const;

private:
    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint EBO = 0;
    std::vector<glm::vec3>    vertices;
    std::vector<unsigned int> indices;
    int                       subdivisions;

    void releaseGL();
};
