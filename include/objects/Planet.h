#pragma once
#include <vector>
#include <glm/glm.hpp>

class Planet
{
public:
    Planet(float radius, int subdivisions);
    ~Planet();
    void render();

private:
    float radius;
    unsigned int VAO, VBO, EBO;
    std::vector<glm::vec3> vertices;
    std::vector<unsigned int> indices;

    void generateIcosahedron();
    void subdivide(int depth);
    void setupMesh();
};
