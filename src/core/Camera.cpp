#include "core/Camera.h"
#include <iostream>

Camera::Camera(glm::vec3 startPosition)
    : position(startPosition), front(glm::vec3(0.0f, 0.0f, -1.0f)), up(glm::vec3(0.0f, 1.0f, 0.0f)),
    yaw(-90.0f), pitch(0.0f), speed(2.5f), sensitivity(0.1f) {
}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(position, position + front, up);
}

void Camera::processKeyboard(int key, float deltaTime) {
    float velocity = speed * deltaTime;
    if (key == GLFW_KEY_W) position += front * velocity;
    if (key == GLFW_KEY_S) position -= front * velocity;
    if (key == GLFW_KEY_A) position -= glm::normalize(glm::cross(front, up)) * velocity;
    if (key == GLFW_KEY_D) position += glm::normalize(glm::cross(front, up)) * velocity;
}

void Camera::processMouseMovement(float xoffset, float yoffset) {
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(direction);
}
