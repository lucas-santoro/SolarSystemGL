#include "Camera.h"

#include <cmath>

Camera::Camera(glm::vec3 startPosition)
    : position_(startPosition)
{
}

glm::mat4 Camera::getViewMatrix()
{
    if (mode_ == CameraMode::ORBITAL && orbitalTargetIndex_ >= 0)
    {
        const float yawRad   = glm::radians(yaw_);
        const float pitchRad = glm::radians(pitch_);
        const float cosPitch = std::cos(pitchRad);

        position_.x = orbitalTargetPos_.x + orbitalDistance_ * cosPitch * std::cos(yawRad);
        position_.y = orbitalTargetPos_.y + orbitalDistance_ * std::sin(pitchRad);
        position_.z = orbitalTargetPos_.z + orbitalDistance_ * cosPitch * std::sin(yawRad);

        return glm::lookAt(position_, orbitalTargetPos_, up_);
    }

    return glm::lookAt(position_, position_ + front_, up_);
}

void Camera::processKeyboard(int key, float deltaTime)
{
    if (mode_ != CameraMode::FREE) return;

    const float velocity = speed_ * deltaTime;
    if (key == GLFW_KEY_W) position_ += front_ * velocity;
    if (key == GLFW_KEY_S) position_ -= front_ * velocity;
    if (key == GLFW_KEY_A) position_ -= glm::normalize(glm::cross(front_, up_)) * velocity;
    if (key == GLFW_KEY_D) position_ += glm::normalize(glm::cross(front_, up_)) * velocity;
}

void Camera::processMouseMovement(float xoffset, float yoffset)
{
    xoffset *= sensitivity_;
    yoffset *= sensitivity_;

    yaw_   += xoffset;
    pitch_ += yoffset;

    constexpr float kMaxPitch = 89.0f;
    if (pitch_ >  kMaxPitch) pitch_ =  kMaxPitch;
    if (pitch_ < -kMaxPitch) pitch_ = -kMaxPitch;

    if (mode_ == CameraMode::FREE)
    {
        glm::vec3 direction;
        direction.x = std::cos(glm::radians(yaw_)) * std::cos(glm::radians(pitch_));
        direction.y = std::sin(glm::radians(pitch_));
        direction.z = std::sin(glm::radians(yaw_)) * std::cos(glm::radians(pitch_));
        front_ = glm::normalize(direction);
    }
}

void Camera::processMouseScroll(float yoffset)
{
    if (mode_ == CameraMode::ORBITAL)
    {
        orbitalDistance_ -= yoffset * (orbitalDistance_ * 0.1f);
        if (orbitalDistance_ < 5.0f) orbitalDistance_ = 5.0f;
    }
    else
    {
        speed_ += yoffset * 10.0f;
        if (speed_ < 10.0f) speed_ = 10.0f;
    }
}

void Camera::setMode(CameraMode newMode)
{
    mode_         = newMode;
    isTravelling_ = false;

    if (mode_ == CameraMode::FREE)
    {
        orbitalTargetIndex_ = -1;
    }
}

CameraMode Camera::getMode() const
{
    return mode_;
}

void Camera::setOrbitalTarget(int index, float initialDistance)
{
    orbitalTargetIndex_ = index;
    orbitalDistance_    = initialDistance;
}

void Camera::setOrbitalTargetPos(const glm::vec3& pos)
{
    orbitalTargetPos_ = pos;
}

int Camera::getOrbitalTargetIndex() const
{
    return orbitalTargetIndex_;
}

void Camera::shiftOrbitalIndexOnRemove(int removedIdx)
{
    if (orbitalTargetIndex_ == removedIdx)
    {
        setMode(CameraMode::FREE);   // also clears orbitalTargetIndex_
    }
    else if (orbitalTargetIndex_ > removedIdx)
    {
        --orbitalTargetIndex_;
    }
}

glm::vec3 Camera::getRayFromMouse(double mouseX, double mouseY,
                                  int screenWidth, int screenHeight,
                                  const glm::mat4& view, const glm::mat4& projection)
{
    const float ndcX = (2.0f * static_cast<float>(mouseX)) / screenWidth  - 1.0f;
    const float ndcY = 1.0f - (2.0f * static_cast<float>(mouseY)) / screenHeight;

    glm::vec4 rayClip(ndcX, ndcY, -1.0f, 1.0f);
    glm::vec4 rayEye = glm::inverse(projection) * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);

    const glm::vec3 rayWorld(glm::inverse(view) * rayEye);
    return glm::normalize(rayWorld);
}

glm::vec3 Camera::getPosition() const
{
    return position_;
}

glm::vec2 Camera::worldToScreen(const glm::vec3& worldPos,
                                const glm::mat4& view, const glm::mat4& projection,
                                int screenWidth, int screenHeight)
{
    const glm::vec4 clipSpacePos = projection * view * glm::vec4(worldPos, 1.0f);
    if (clipSpacePos.w <= 0.0f) return { -1.0f, -1.0f };

    const glm::vec3 ndcSpacePos = glm::vec3(clipSpacePos) / clipSpacePos.w;
    const float x = (ndcSpacePos.x * 0.5f + 0.5f) * screenWidth;
    const float y = (1.0f - (ndcSpacePos.y * 0.5f + 0.5f)) * screenHeight;
    return { x, y };
}

void Camera::startSmoothMove(const glm::vec3& destination, float distance)
{
    targetPos_    = destination - front_ * distance;
    isTravelling_ = true;
}

void Camera::update(float dt)
{
    if (!isTravelling_) return;

    const glm::vec3 diff = targetPos_ - position_;
    const float     dist = glm::length(diff);

    constexpr float kArrivalEpsilon = 0.1f;
    if (dist < kArrivalEpsilon)
    {
        position_     = targetPos_;
        isTravelling_ = false;
        return;
    }

    glm::vec3 step = glm::normalize(diff) * travelSpeed_ * dt;
    if (glm::length(step) > dist) step = diff;
    position_ += step;
}

void Camera::reset()
{
    position_     = glm::vec3(0.0f, 0.0f, 300.0f);
    front_        = glm::vec3(0.0f, 0.0f, -1.0f);
    up_           = glm::vec3(0.0f, 1.0f, 0.0f);
    yaw_          = -90.0f;
    pitch_        = 0.0f;
    isTravelling_ = false;
    setMode(CameraMode::FREE);
}
