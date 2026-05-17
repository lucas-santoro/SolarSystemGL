#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>

#include <optional>

/**
 * @file Camera.h
 * @brief Two-mode (FREE / ORBITAL) 3D camera with smooth-move and picking helpers.
 */

/**
 * @brief Camera operating mode.
 */
enum class CameraMode
{
    FREE,    /**< WASD movement, mouse look. position/front/yaw/pitch live freely. */
    ORBITAL  /**< Locked to a body, orbits around it via yaw/pitch/distance. */
};

/**
 * @brief 3D camera with two operating modes and helpers for picking and smooth movement.
 *
 * Fully encapsulates pose state (position, orientation, mode, orbital target).
 * Callers manipulate it through behavior methods only — no direct field access.
 */
class Camera
{
public:
    /**
     * @brief Construct the camera with an initial world-space position.
     * @param startPosition Initial position in world units.
     */
    explicit Camera(glm::vec3 startPosition);

    /// @return The view matrix corresponding to the current mode and pose.
    glm::mat4 getViewMatrix();

    /**
     * @brief Cast a ray from the camera through a screen-space point.
     * @param mouseX        Mouse X in screen pixels.
     * @param mouseY        Mouse Y in screen pixels.
     * @param screenWidth   Window width in pixels.
     * @param screenHeight  Window height in pixels.
     * @param view          Current view matrix.
     * @param projection    Current projection matrix.
     * @return              Normalized world-space ray direction.
     */
    glm::vec3 getRayFromMouse(double mouseX, double mouseY,
                              int screenWidth, int screenHeight,
                              const glm::mat4& view, const glm::mat4& projection);

    /// @return The current world-space camera position.
    glm::vec3 getPosition() const;

    /**
     * @brief Project a world point to screen pixel coordinates.
     * @return `(x, y)` pixel position, or `(-1, -1)` if the point projects behind the camera.
     */
    glm::vec2 worldToScreen(const glm::vec3& worldPos,
                            const glm::mat4& view, const glm::mat4& projection,
                            int screenWidth, int screenHeight);

    /// @return The current operating mode.
    CameraMode getMode() const;

    /// Switch operating mode. Setting FREE clears any orbital target.
    void setMode(CameraMode newMode);

    /// @return Index of the body the orbital camera is tracking, or -1 if none.
    int getOrbitalTargetIndex() const;

    /**
     * @brief Lock the orbital camera onto a body by index.
     * @param index           Index into the bodies vector.
     * @param initialDistance Initial orbital radius in world units.
     */
    void setOrbitalTarget(int index, float initialDistance);

    /**
     * @brief Cache the world-space position of the orbital target.
     *
     * Called once per frame by the host loop, before #getViewMatrix, so the
     * camera tracks the body as it moves.
     */
    void setOrbitalTargetPos(const glm::vec3& pos);

    /**
     * @brief Adjust the orbital target index after a body has been removed.
     *
     * Either clears the target (if the removed body *was* the target) or shifts
     * the index down by one (if the removed body was at a lower index).
     */
    void shiftOrbitalIndexOnRemove(int removedIdx);

    /// Handle a mouse scroll event — zoom in ORBITAL, change speed in FREE.
    void processMouseScroll(float yoffset);

    /// Handle a held WASD key for movement (FREE mode only).
    void processKeyboard(int key, float deltaTime);

    /// Handle mouse-look delta — adjusts yaw/pitch, recomputes the front vector.
    void processMouseMovement(float xoffset, float yoffset);

    /// Start a smooth interpolation toward @p destination keeping a given offset distance.
    void startSmoothMove(const glm::vec3& destination, float distance = 60.0f);

    /**
     * @brief Smoothly fly to an orbital pose around @p targetPos and switch to ORBITAL on arrival.
     *
     * Derives yaw/pitch from the current camera→target direction, computes the
     * final orbital position at @p distance away, then drives a smooth move
     * (FREE mode during the flight, ORBITAL committed on arrival). Use this
     * instead of `setMode(ORBITAL) + setOrbitalTarget(...)` to avoid the snap
     * caused by the orbital branch immediately rewriting `position_`.
     *
     * @param targetIndex Index of the target body (cached for hover/remove tracking).
     * @param targetPos   World-space position of the target body.
     * @param distance    Orbital radius the camera should settle at.
     */
    void flyToOrbital(int targetIndex, const glm::vec3& targetPos, float distance);

    /// Advance any active smooth movement by @p dt seconds.
    void update(float dt);

    /// Restore the camera to its startup pose and FREE mode.
    void reset();

    /// @return Mouse-look sensitivity multiplier (degrees per pixel-equivalent).
    float getSensitivity() const { return sensitivity_; }

    /// Set the mouse-look sensitivity multiplier.
    void setSensitivity(float value) { sensitivity_ = value; }

private:
    // Pose (FREE mode is authoritative; ORBITAL rewrites these from spherical coords).
    glm::vec3 position_;
    glm::vec3 front_{ 0.0f, 0.0f, -1.0f };
    glm::vec3 up_{ 0.0f, 1.0f, 0.0f };
    float     yaw_         = -90.0f;
    float     pitch_       = 0.0f;
    float     speed_       = 80.0f;
    float     sensitivity_ = 0.1f;

    // Smooth-move state.
    bool      isTravelling_ = false;
    glm::vec3 targetPos_{ 0.0f };
    float     travelSpeed_  = 3000.0f;

    // Mode + orbital state.
    CameraMode                mode_                = CameraMode::FREE;
    int                       orbitalTargetIndex_  = -1;
    glm::vec3                 orbitalTargetPos_{ 0.0f };
    float                     orbitalDistance_     = 100.0f;
    std::optional<CameraMode> pendingMode_;  ///< Mode to apply when an in-flight smooth move arrives.
};
