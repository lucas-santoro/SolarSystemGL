#pragma once
#include <vector>
#include <glm/glm.hpp>

class Planet
{
public:
    Planet(float radius, int subdivisions);
    ~Planet();
    void render();

    bool intersectsRay(const glm::vec3 &rayOrigin, const glm::vec3 &rayDirection) const;

    glm::vec3 getPosition() const { return position; }
    float getRadius() const { return radius; }

private:
    unsigned int VAO, VBO, EBO;
    std::vector<glm::vec3> vertices;
    std::vector<unsigned int> indices;

    glm::vec3 position;
    float radius;

    void generateIcosahedron();
    void subdivide(int depth);
    void setupMesh();
};
