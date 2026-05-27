#pragma once

#include <glm/glm.hpp>

#include <deque>
#include <string>

/**
 * @file Toast.h
 * @brief Transient bottom-right notification overlay.
 */

/**
 * @brief A single toast message — text, remaining lifetime, and tint.
 */
struct Toast
{
    std::string message; ///< Display text.
    float remaining;     ///< Seconds left before the toast disappears.
    float totalDuration; ///< Initial duration, used to drive the fade-out alpha.
    glm::vec4 color;     ///< Base RGBA before fade-out is applied.
};

/**
 * @brief Owns the active toast queue and renders it via ImGui.
 *
 * Toasts stack bottom-right, oldest at the top of the stack. Each entry fades
 * out during its last 0.5s. The host loop calls #update each frame to tick
 * timers and #render to draw remaining toasts.
 *
 * Convenience helpers (#info, #success, #warning, #error) wrap #push with
 * preset colors so call sites stay readable.
 */
class ToastQueue
{
public:
    /// Default toast lifetime in seconds.
    static constexpr float kDefaultDuration = 4.0f;

    /**
     * @brief Enqueue a toast with custom color and duration.
     * @param message  Display text.
     * @param color    RGBA tint applied to the toast background.
     * @param duration Seconds before the toast is removed (fade begins at last 0.5s).
     */
    void push(const std::string& message, const glm::vec4& color, float duration = kDefaultDuration);

    /// Enqueue a neutral white toast.
    void info(const std::string& message);

    /// Enqueue a green success toast.
    void success(const std::string& message);

    /// Enqueue a yellow warning toast.
    void warning(const std::string& message);

    /// Enqueue a red error toast.
    void error(const std::string& message);

    /// Tick remaining timers and drop expired toasts. Call once per frame.
    void update(float deltaTime);

    /// Draw all active toasts. Must run between ImGui::NewFrame and ImGui::Render.
    void render();

    /// Remove every active toast immediately (e.g., when returning to the menu).
    void clear();

private:
    std::deque<Toast> toasts_;
};
