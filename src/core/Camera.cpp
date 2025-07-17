#include "Camera.h"
#include <iostream>

Camera::Camera(glm::vec3 startPosition)
    : position(startPosition), front(glm::vec3(0.0f, 0.0f, -1.0f)), up(glm::vec3(0.0f, 1.0f, 0.0f)),
    yaw(-90.0f), pitch(0.0f), speed(80.0f), sensitivity(0.1f) {
}

glm::mat4 Camera::getViewMatrix() {
    if (mode == CameraMode::ORBITAL && orbitalTarget != nullptr) {
        float camX = orbitalTarget->x + orbitalDistance * cos(glm::radians(pitch)) * cos(glm::radians(yaw));
        float camY = orbitalTarget->y + orbitalDistance * sin(glm::radians(pitch));
        float camZ = orbitalTarget->z + orbitalDistance * cos(glm::radians(pitch)) * sin(glm::radians(yaw));

        this->position = glm::vec3(camX, camY, camZ);

        return glm::lookAt(this->position, *orbitalTarget, this->up);
    }

    return glm::lookAt(position, position + front, up);
}

void Camera::processKeyboard(int key, float deltaTime) {
    if (mode == CameraMode::FREE) 
    {
        float velocity = speed * deltaTime;
        if (key == GLFW_KEY_W) position += front * velocity;
        if (key == GLFW_KEY_S) position -= front * velocity;
        if (key == GLFW_KEY_A) position -= glm::normalize(glm::cross(front, up)) * velocity;
        if (key == GLFW_KEY_D) position += glm::normalize(glm::cross(front, up)) * velocity;
    }
}

void Camera::processMouseMovement(float xoffset, float yoffset) 
{
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    if (mode == CameraMode::FREE) 
    {
        glm::vec3 direction;
        direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        direction.y = sin(glm::radians(pitch));
        direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(direction);
    }
}

void Camera::processMouseScroll(float yoffset) 
{
    if (mode == CameraMode::ORBITAL) {
        orbitalDistance -= yoffset * (orbitalDistance * 0.1f);
        if (orbitalDistance < 5.0f) orbitalDistance = 5.0f;
    }
    else {
        speed += yoffset * 10.0f;
        if (speed < 10.0f) speed = 10.0f;
    }
}

void Camera::setMode(CameraMode newMode) 
{
    mode = newMode;
    isTravelling = false;

    if (mode == CameraMode::FREE) 
    {
        orbitalTarget = nullptr; 
    }
}

CameraMode Camera::getMode() const 
{
    return mode;
}

void Camera::setOrbitalTarget(const glm::vec3* targetPosition, float initialDistance) 
{
    orbitalTarget = targetPosition;
    orbitalDistance = initialDistance;
}

glm::vec3 Camera::getRayFromMouse(double mouseX, double mouseY, int screenWidth, int screenHeight, const glm::mat4 &view, const glm::mat4 &projection)
{
    float x = (2.0f * mouseX) / screenWidth - 1.0f;
    float y = 1.0f - (2.0f * mouseY) / screenHeight;
    glm::vec4 rayClip = glm::vec4(x, y, -1.0f, 1.0f);

    glm::vec4 rayEye = glm::inverse(projection) * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);

    glm::vec3 rayWorld = glm::vec3(glm::inverse(view) * rayEye);
    return glm::normalize(rayWorld);
}

glm::vec3 Camera::getPosition()
{
	return position;
}

glm::vec2 Camera::worldToScreen(const glm::vec3 &worldPos, const glm::mat4 &view, const glm::mat4 &projection, int screenWidth, int screenHeight)
{
    glm::vec4 clipSpacePos = projection * view * glm::vec4(worldPos, 1.0f);
    if (clipSpacePos.w <= 0.0f) return glm::vec2(-1.0f, -1.0f);

    glm::vec3 ndcSpacePos = glm::vec3(clipSpacePos) / clipSpacePos.w;
    float x = (ndcSpacePos.x * 0.5f + 0.5f) * screenWidth;
    float y = (1.0f - (ndcSpacePos.y * 0.5f + 0.5f)) * screenHeight;

    return glm::vec2(x, y);
}

void Camera::startSmoothMove(const glm::vec3& destination, float distance)
{
    targetPos = destination - front * distance;
    isTravelling = true;
}

void Camera::update(float dt)
{
    if (!isTravelling) return;

    glm::vec3 diff = targetPos - position;
    float     dist = glm::length(diff);

    if (dist < 0.1f)
    {
        position = targetPos;
        isTravelling = false;
        return;
    }

    glm::vec3 step = glm::normalize(diff) * travelSpeed * dt;
    if (glm::length(step) > dist) step = diff;

    position += step;
}

const glm::vec3 *Camera::getOrbitalTarget() const {
    return orbitalTarget;
}