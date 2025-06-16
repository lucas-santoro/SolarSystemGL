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
    glm::vec3 getRayFromMouse(double mouseX, double mouseY, int screenWidth, int screenHeight, const glm::mat4 &view, const glm::mat4 &projection);
    glm::vec3 getPosition();
    glm::vec2 worldToScreen(const glm::vec3& worldPos, const glm::mat4& view, const glm::mat4& projection, int screenWidth, int screenHeight);

    void processKeyboard(int key, float deltaTime);
    void processMouseMovement(float xoffset, float yoffset);

    void startSmoothMove(const glm::vec3 &destination, float distance = 60.0f);
    void update(float dt);

private:
    bool  isTravelling = false;
    glm::vec3 targetPos;
    float travelSpeed = 3000.0f;
};
