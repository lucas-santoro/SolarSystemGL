#pragma once
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "core/Shader.h"

class Planet
{
public:
    Planet(const std::string &name, float mass, float density, glm::vec3 position, glm::vec3 velocity, glm::vec3 color, int subdivisions = 3);
    ~Planet();

    void render(Shader &shader, bool highlight = false);
    bool intersectsRay(const glm::vec3 &rayOrigin, const glm::vec3 &rayDirection) const;

    std::string getName() const     { return name; }
    glm::vec3 getPosition() const   { return position; }
    glm::vec3 getVelocity() const   { return velocity; }
    float getRadius() const         { return radius; }
    float getMass() const           { return mass; }
    float getDensity() const        { return density; }
    float getPickRadius() const     { return std::max(radius, MIN_PICK_RADIUS); }

    void setPosition(const glm::vec3 &newPosition);
    void setVelocity(const glm::vec3 &newVelocity);
    void setMass(float newMass);
    void setDensity(float newDensity);
    void setName(const std::string& newName);

    void recalculateGeometry();
private:
    unsigned int VAO, VBO, EBO;
    static constexpr float MIN_PICK_RADIUS = 18.0f;

    std::vector<glm::vec3> vertices;
    std::vector<unsigned int> indices;
    int subdivisions;

    std::string name;
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 color;
    float radius;
    float mass;
    float density;

    void generateIcosahedron();
    void subdivide(int depth);
    void setupMesh();
    void calculateRadius();
};