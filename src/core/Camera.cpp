#include "Camera.h"

#include <cmath>

Camera::Camera(glm::vec3 startPosition) : position_(startPosition)
{
    // Compute front_ from member-default yaw/pitch so the initial view
    // matches what `getViewMatrix` will produce. Without this, position_
    // and yaw_/pitch_ would disagree until the first reset().
    glm::vec3 direction;
    direction.x = std::cos(glm::radians(yaw_)) * std::cos(glm::radians(pitch_));
    direction.y = std::sin(glm::radians(pitch_));
    direction.z = std::sin(glm::radians(yaw_)) * std::cos(glm::radians(pitch_));
    front_      = glm::normalize(direction);
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
    if (mode_ != CameraMode::FREE)
        return;

    const float velocity = speed_ * deltaTime;
    if (key == GLFW_KEY_W)
        position_ += front_ * velocity;
    if (key == GLFW_KEY_S)
        position_ -= front_ * velocity;
    if (key == GLFW_KEY_A)
        position_ -= glm::normalize(glm::cross(front_, up_)) * velocity;
    if (key == GLFW_KEY_D)
        position_ += glm::normalize(glm::cross(front_, up_)) * velocity;
}

void Camera::processMouseMovement(float xoffset, float yoffset)
{
    xoffset *= sensitivity_;
    yoffset *= sensitivity_;

    // Yaw follows the same horizontal convention in both modes (drag right →
    // viewport rotates right). Pitch is inverted in ORBITAL so dragging up
    // tilts the view up over the body — what feels natural when the camera
    // is anchored to a target.
    yaw_ += xoffset;
    if (mode_ == CameraMode::ORBITAL)
    {
        pitch_ -= yoffset;
    }
    else
    {
        pitch_ += yoffset;
    }

    constexpr float kMaxPitch = 89.0f;
    if (pitch_ > kMaxPitch)
        pitch_ = kMaxPitch;
    if (pitch_ < -kMaxPitch)
        pitch_ = -kMaxPitch;

    if (mode_ == CameraMode::FREE)
    {
        glm::vec3 direction;
        direction.x = std::cos(glm::radians(yaw_)) * std::cos(glm::radians(pitch_));
        direction.y = std::sin(glm::radians(pitch_));
        direction.z = std::sin(glm::radians(yaw_)) * std::cos(glm::radians(pitch_));
        front_      = glm::normalize(direction);
    }
}

void Camera::processMouseScroll(float yoffset)
{
    if (mode_ == CameraMode::ORBITAL)
    {
        orbitalDistance_ -= yoffset * (orbitalDistance_ * 0.1f);
        if (orbitalDistance_ < 5.0f)
            orbitalDistance_ = 5.0f;
    }
    else
    {
        speed_ += yoffset * 10.0f;
        if (speed_ < 10.0f)
            speed_ = 10.0f;
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
        setMode(CameraMode::FREE); // also clears orbitalTargetIndex_
    }
    else if (orbitalTargetIndex_ > removedIdx)
    {
        --orbitalTargetIndex_;
    }
}

glm::vec3 Camera::getRayFromMouse(double mouseX, double mouseY, int screenWidth, int screenHeight,
                                  const glm::mat4& view, const glm::mat4& projection)
{
    const float ndcX = (2.0f * static_cast<float>(mouseX)) / screenWidth - 1.0f;
    const float ndcY = 1.0f - (2.0f * static_cast<float>(mouseY)) / screenHeight;

    glm::vec4 rayClip(ndcX, ndcY, -1.0f, 1.0f);
    glm::vec4 rayEye = glm::inverse(projection) * rayClip;
    rayEye           = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);

    const glm::vec3 rayWorld(glm::inverse(view) * rayEye);
    return glm::normalize(rayWorld);
}

glm::vec3 Camera::getPosition() const
{
    return position_;
}

glm::vec2 Camera::worldToScreen(const glm::vec3& worldPos, const glm::mat4& view, const glm::mat4& projection,
                                int screenWidth, int screenHeight)
{
    const glm::vec4 clipSpacePos = projection * view * glm::vec4(worldPos, 1.0f);
    if (clipSpacePos.w <= 0.0f)
        return {-1.0f, -1.0f};

    const glm::vec3 ndcSpacePos = glm::vec3(clipSpacePos) / clipSpacePos.w;
    const float x               = (ndcSpacePos.x * 0.5f + 0.5f) * screenWidth;
    const float y               = (1.0f - (ndcSpacePos.y * 0.5f + 0.5f)) * screenHeight;
    return {x, y};
}

void Camera::startSmoothMove(const glm::vec3& destination, float distance)
{
    targetPos_    = destination - front_ * distance;
    isTravelling_ = true;
}

void Camera::flyToPose(const glm::vec3& position, float yaw, float pitch)
{
    // Capture the starting pose so update(dt) can interpolate alongside the
    // position move via the same progress fraction.
    poseStartPos_    = position_;
    poseStartYaw_    = yaw_;
    poseStartPitch_  = pitch_;
    poseTargetYaw_   = yaw;
    poseTargetPitch_ = pitch;
    hasPendingPose_  = true;

    targetPos_    = position;
    isTravelling_ = true;

    // Pose preset is a free-fly target — drop any orbital tracking.
    mode_               = CameraMode::FREE;
    orbitalTargetIndex_ = -1;
    pendingMode_.reset();
}

void Camera::flyToOrbital(int targetIndex, const glm::vec3& targetPos, float distance)
{
    orbitalTargetIndex_ = targetIndex;
    orbitalTargetPos_   = targetPos;
    orbitalDistance_    = distance;

    // Derive yaw/pitch from the current camera→target direction so the
    // post-arrival orbital view aligns with the camera's current vantage point.
    const glm::vec3 toCam = glm::normalize(position_ - targetPos);
    yaw_                  = glm::degrees(std::atan2(toCam.z, toCam.x));
    pitch_                = glm::degrees(std::asin(toCam.y));

    constexpr float kMaxPitch = 89.0f;
    if (pitch_ > kMaxPitch)
        pitch_ = kMaxPitch;
    if (pitch_ < -kMaxPitch)
        pitch_ = -kMaxPitch;

    const float yawRad   = glm::radians(yaw_);
    const float pitchRad = glm::radians(pitch_);
    const float cosPitch = std::cos(pitchRad);
    targetPos_ = targetPos + glm::vec3(distance * cosPitch * std::cos(yawRad), distance * std::sin(pitchRad),
                                       distance * cosPitch * std::sin(yawRad));

    // Stay in FREE during the flight so getViewMatrix doesn't immediately
    // snap position_ via the spherical formula; commit ORBITAL on arrival.
    mode_         = CameraMode::FREE;
    pendingMode_  = CameraMode::ORBITAL;
    isTravelling_ = true;
}

void Camera::update(float dt)
{
    if (!isTravelling_)
        return;

    // While flying toward an orbital pose, aim the camera at the target so the
    // mode flip on arrival doesn't introduce a view snap.
    if (pendingMode_.has_value() && *pendingMode_ == CameraMode::ORBITAL)
    {
        const glm::vec3 toTarget = orbitalTargetPos_ - position_;
        const float distance     = glm::length(toTarget);
        if (distance > 0.001f)
            front_ = toTarget / distance;
    }

    // Pose interpolation — lerp yaw/pitch (and the derived front vector)
    // alongside the position move so the camera smoothly tilts as it glides.
    if (hasPendingPose_)
    {
        const glm::vec3 totalSpan     = targetPos_ - poseStartPos_;
        const float totalSpanLen      = glm::length(totalSpan);
        const glm::vec3 remainingSpan = targetPos_ - position_;
        const float remainingLen      = glm::length(remainingSpan);
        const float progress =
            (totalSpanLen > 0.01f) ? glm::clamp(1.0f - remainingLen / totalSpanLen, 0.0f, 1.0f) : 1.0f;

        yaw_   = glm::mix(poseStartYaw_, poseTargetYaw_, progress);
        pitch_ = glm::mix(poseStartPitch_, poseTargetPitch_, progress);

        glm::vec3 direction;
        direction.x = std::cos(glm::radians(yaw_)) * std::cos(glm::radians(pitch_));
        direction.y = std::sin(glm::radians(pitch_));
        direction.z = std::sin(glm::radians(yaw_)) * std::cos(glm::radians(pitch_));
        front_      = glm::normalize(direction);
    }

    const glm::vec3 diff = targetPos_ - position_;
    const float dist     = glm::length(diff);

    constexpr float kArrivalEpsilon = 0.1f;
    if (dist < kArrivalEpsilon)
    {
        position_     = targetPos_;
        isTravelling_ = false;

        if (hasPendingPose_)
        {
            yaw_   = poseTargetYaw_;
            pitch_ = poseTargetPitch_;
            glm::vec3 direction;
            direction.x     = std::cos(glm::radians(yaw_)) * std::cos(glm::radians(pitch_));
            direction.y     = std::sin(glm::radians(pitch_));
            direction.z     = std::sin(glm::radians(yaw_)) * std::cos(glm::radians(pitch_));
            front_          = glm::normalize(direction);
            hasPendingPose_ = false;
        }

        if (pendingMode_.has_value())
        {
            mode_ = *pendingMode_;
            pendingMode_.reset();
        }
        return;
    }

    glm::vec3 step = glm::normalize(diff) * travelSpeed_ * dt;
    if (glm::length(step) > dist)
        step = diff;
    position_ += step;
}

void Camera::reset()
{
    // Cinematic 3/4 angle — upper-front-right looking down at the origin.
    // Frames the inner Solar System bodies nicely on first load and provides
    // a more interesting default than the flat (0, 0, 300) front view.
    position_ = glm::vec3(220.0f, 160.0f, 280.0f);
    yaw_      = -128.0f;
    pitch_    = -24.0f;
    up_       = glm::vec3(0.0f, 1.0f, 0.0f);

    glm::vec3 direction;
    direction.x = std::cos(glm::radians(yaw_)) * std::cos(glm::radians(pitch_));
    direction.y = std::sin(glm::radians(pitch_));
    direction.z = std::sin(glm::radians(yaw_)) * std::cos(glm::radians(pitch_));
    front_      = glm::normalize(direction);

    isTravelling_   = false;
    hasPendingPose_ = false;
    setMode(CameraMode::FREE);
}
