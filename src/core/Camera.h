#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>

enum class CameraMode {
    FREE,
    ORBITAL
};

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

    glm::mat4 getViewMatrix();
    glm::vec3 getRayFromMouse(double mouseX, double mouseY, int screenWidth, int screenHeight, const glm::mat4 &view, const glm::mat4 &projection);
    glm::vec3 getPosition();
    glm::vec2 worldToScreen(const glm::vec3 &worldPos, const glm::mat4 &view, const glm::mat4 &projection, int screenWidth, int screenHeight);
    CameraMode getMode() const;
    void setMode(CameraMode newMode);
    const glm::vec3* getOrbitalTarget() const;
    void setOrbitalTarget(const glm::vec3 *targetPosition, float initialDistance);
    void processMouseScroll(float yoffset);

    void processKeyboard(int key, float deltaTime);
    void processMouseMovement(float xoffset, float yoffset);

    void startSmoothMove(const glm::vec3 &destination, float distance = 60.0f);
    void update(float dt);

private:
    bool  isTravelling = false;
    glm::vec3 targetPos;
    float travelSpeed = 3000.0f;

    CameraMode mode = CameraMode::FREE;
    const glm::vec3* orbitalTarget = nullptr;
    float orbitalDistance = 100.0f;
};
