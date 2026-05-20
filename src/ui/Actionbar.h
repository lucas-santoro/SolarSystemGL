#pragma once

#include <vector>

class Camera;
class PhysicsSystem;
class UIManager;
class Window;
struct CelestialBody;
struct GridSettings;

/**
 * @file Actionbar.h
 * @brief Top + bottom in-game GUI strips. Hosts the primary simulation controls.
 */

/**
 * @brief Two ImGui side-bars that consolidate the in-game controls.
 *
 * Layout:
 * - **Top bar**: menu/system dropdown, time controls, camera controls, save/
 *   load/settings, FPS.
 * - **Bottom bar**: visual toggles (trails, bloom, diagnostics, vsync).
 *
 * The bars mutate `Camera`, `PhysicsSystem`, and various `UIManager` flags
 * directly. "Open modal" intents (menu, settings, save, load) are written to
 * boolean flags on the `UIManager` so the host loop can decide what to do
 * with them (e.g., open a popup, run a save routine).
 */
class Actionbar
{
public:
    /// Pixel height of the top side-bar. Exposed so panels can offset below it.
    static constexpr float kTopBarHeight    = 40.0f;

    /// Pixel height of the bottom side-bar.
    static constexpr float kBottomBarHeight = 32.0f;

    /**
     * @brief Render the top side-bar for the current frame.
     *
     * Must be called between ImGui::NewFrame and ImGui::Render.
     */
    void renderTopBar(Window& window, Camera& camera, PhysicsSystem& physics,
                      std::vector<CelestialBody>& bodies, UIManager& uiManager,
                      float deltaTime);

    /**
     * @brief Render the bottom side-bar for the current frame.
     *
     * @param gridSettings Read/write — the bottom bar binds the Grid
     *                     visibility checkbox directly to `gridSettings.visible`.
     */
    void renderBottomBar(Window& window, UIManager& uiManager, GridSettings& gridSettings);

private:
    /// Exponentially-smoothed FPS readout shown on the right of the top bar.
    float smoothedFps_ = 60.0f;
};
