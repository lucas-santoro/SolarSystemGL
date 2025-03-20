#pragma once
#include <vector>
#include <glm/glm.hpp>

class Planet
{
public:
    Planet(float mass, float density, glm::vec3 position, glm::vec3 velocity, int subdivisions = 3);
    ~Planet();
    void render();

    bool intersectsRay(const glm::vec3 &rayOrigin, const glm::vec3 &rayDirection) const;

    glm::vec3 getPosition() const { return position; }
    glm::vec3 getVelocity() const { return velocity; }
    float getRadius() const       { return radius; }
    float getMass() const         { return mass; }
    float getDensity() const      { return density; }

    void setPosition(const glm::vec3 &newPosition);
    void setVelocity(const glm::vec3 &newVelocity);
    void setMass(float newMass);
    void setDensity(float newDensity);

private:
    unsigned int VAO, VBO, EBO;
    std::vector<glm::vec3> vertices;
    std::vector<unsigned int> indices;

    glm::vec3 position;
    float radius;
    glm::vec3 velocity;
    float mass;
    float density;

    void generateIcosahedron();
    void subdivide(int depth);
    void setupMesh();
    void calculateRadius();
};
