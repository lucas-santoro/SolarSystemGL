#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>

class Camera {
public:
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;

    float yaw;
    float pitch;
    float speed;
    float sensitivity;

    Camera(glm::vec3 startPosition);

    glm::mat4 getViewMatrix() const;
    void processKeyboard(int key, float deltaTime);
    void processMouseMovement(float xoffset, float yoffset);
};
