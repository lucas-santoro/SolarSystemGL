#pragma once

#include <string>
#include <vector>
#include "objects/CelestialBody.h"
#include "core/Window.h"
#include "core/Camera.h"
#include "core/Grid.h"
#include "physics/PhysicsSystem.h"
#include "ui/Actionbar.h"
#include "ui/Toast.h"

/**
 * @file UIManager.h
 * @brief ImGui-driven user interface: panels, picking, edit buffer.
 */

/**
 * @brief Owns all ImGui panels and the picking pass.
 *
 * Stateful across frames — holds the current selection, hover index, edit
 * buffer for the selected body, and assorted flags (paused, vsync, show
 * trails/bloom). Panels mutate the simulation state through references passed
 * to #render, e.g., the Apply Changes button writes back to the selected body
 * directly.
 *
 * A handful of fields are intentionally public so ImGui widgets can bind to
 * them by pointer (e.g., `ImGui::Checkbox(&vsync)`). Encapsulating them behind
 * getter/setter pairs would force copy-and-write-back gymnastics with no
 * actual safety gain.
 */
class UIManager
{
public:
    /**
     * @brief Render every panel for one frame.
     *
     * Order: navbar → picking pass (hover label, click-to-select) → main panel
     * (FPS, presets, save/load, time scale, Add Planet, diagnostics) → planet
     * info panel (only if something is selected) → deferred body-removal if
     * the Remove button was pressed this frame.
     *
     * @param window     Active window (used for cursor position and framebuffer size).
     * @param camera     Camera — modified by smooth-focus and orbital target switches.
     * @param deltaTime  Frame delta in seconds (drives the FPS readout).
     * @param bodies     Mutable body vector — may be mutated, appended to, or shrunk.
     * @param grid       Grid (currently passed through for future grid controls).
     * @param physics    Physics system — `paused` and `timeScale` are bound to UI widgets.
     */
    void render(Window& window, Camera& camera, float deltaTime,
                std::vector<CelestialBody>& bodies, Grid& grid, PhysicsSystem& physics,
                GridSettings& gridSettings);

    /** @return `true` if the right mouse button is held and ImGui is not capturing the mouse. */
    bool isRightMousePressed(GLFWwindow* window);

    /** @return `true` if the body at index @p i is the current hover target. */
    bool isHovered(size_t i) const { return static_cast<int>(i) == hoveredIndex; }

    /** @return Index of the currently selected body, or -1 if none. */
    int getSelectedPlanetIndex() const { return selectedPlanetIndex; }

    /** @brief Clear the current selection (no Planet Info panel will draw next frame). */
    void clearSelection()
    {
        selectedPlanetIndex = -1;
        lastSelectedIndex   = -1;
        hoveredIndex        = -1;
    }

    /** @brief Programmatically select the body at index @p i (or clear with -1). */
    void setSelected(int i) { selectedPlanetIndex = i; }

    /** @brief Push a toast notification onto the in-game notification stack. */
    ToastQueue& toasts() { return toasts_; }

    /** @return `true` while the user is mid-drag placing a new body. */
    bool isPlacingBody() const { return placementActive_; }

    /** @return Start position of the active drag-to-place in world units. */
    glm::vec3 getPlacementStart() const { return placementStartPosWU_; }

    /** @return Current cursor world-unit position during an active drag-to-place. */
    glm::vec3 getPlacementEnd() const { return placementEndPosWU_; }

    /** @brief VSync state — toggled by the main-panel checkbox. */
    bool vsync = true;

    /**
     * @brief Set to true by the VSync checkbox handler. The host loop reads it
     *        after each #render and calls `glfwSwapInterval` if set, then clears it.
     */
    bool vsyncDirty = false;

    /** @brief Render-toggle for orbit trails (host loop checks before drawing trails). */
    bool showTrails = true;

    /** @brief Render-toggle for emissive halos (fake bloom). */
    bool showBloom = true;

    /** @brief Render-toggle for the selected body's predicted future trajectory. */
    bool showPathPrediction = false;

    /** @brief Render-toggle for the five Lagrange-point markers between the most massive body and the selection. */
    bool showLagrange = false;

    /** @brief Persistent name-labels above every body (off by default; hover label always works). */
    bool showBodyLabels = false;

    /** @brief Visibility flag for the diagnostics floating panel (bound to a toggle in the bottom actionbar). */
    bool showDiagnostics = false;

    /** @brief Output flag set when the actionbar's "Menu" button is pressed; host loop reads and clears. */
    bool menuRequested = false;

    /** @brief Output flag set when the actionbar's "Settings" button is pressed. */
    bool settingsRequested = false;

    /** @brief Output flag set when the actionbar's "Save" button is pressed. */
    bool saveRequested = false;

    /** @brief Output flag set when the actionbar's "Load" button is pressed. */
    bool loadRequested = false;

    /** @brief Output flag set when the actionbar's "Add Planet" button is pressed. */
    bool addPlanetRequested = false;

    /** @brief Output flag set when the actionbar's "?" Help button is pressed. */
    bool helpRequested = false;

    /** @brief Output flag set when the mobile overflow's "Screenshot" item is
     *         tapped (mirrors the F12 hotkey path for touch-only devices).
     *         The host loop captures the framebuffer and clears the flag. */
    bool screenshotRequested = false;

    /** @brief When true, the host loop hides every UI panel and drives the
     *         camera in a slow cinematic orbit ("Demo Mode"). F11 toggles. */
    bool demoModeActive = false;

    /** @brief When true, the scene FBO is constructed with MSAA samples > 1.
     *         Defaults to true on desktop and false on mobile (set by
     *         `Application::Application` based on `platform::isMobileDevice`).
     *         Settings modal exposes this as a checkbox; flipping it sets
     *         `msaaDirty` so the host loop can rebuild the FBO between frames.
     *
     *         Note: bloom toggle lives on the bottom actionbar (`showBloom`) —
     *         Settings only owns the more expensive MSAA switch. */
    bool msaaEnabled = true;

    /** @brief Set by the Settings modal whenever `msaaEnabled` changes. The
     *         host loop reads it after each render, calls `rebuildSceneFbo`,
     *         then clears the flag. */
    bool msaaDirty = false;

    /**
     * @brief Set by the actionbar's View combo: 0 = Default, 1 = Top-down,
     *        2 = Side-on; -1 means no pending request. Host loop reads this
     *        each frame, calls `Application::applyViewPreset`, and resets.
     */
    int viewPresetRequested = -1;

private:
    int   selectedPlanetIndex = -1;
    int   hoveredIndex        = -1;
    int   lastSelectedIndex   = -1;
    bool  isMouseMoving       = false;
    float smoothedFps         = 60.0f;

    ToastQueue toasts_;

    bool pendingRemove = false;

    // Drag-to-place state. LMB-click in empty 3D space starts a drag; the
    // release point + drag vector determine the new body's position and
    // initial velocity.
    bool      placementActive_     = false;
    glm::vec3 placementStartPosWU_{ 0.0f };
    glm::vec3 placementEndPosWU_  { 0.0f };

    // Left-click body selection state. The fly-to is deferred from press to
    // release so a touch-drag that *starts* on a planet doesn't fire the
    // select+fly intent — matches the RMB context-menu pattern below.
    int  lmbPressedTargetIdx_   = -1;  ///< Body hovered when LMB was first pressed.

    // Right-click context menu state.
    int  rmbPressedTargetIdx_   = -1;  ///< Body hovered when RMB was first pressed.
    int  contextMenuTargetIdx_  = -1;  ///< Body the popup is acting on.
    bool openContextMenuPopup_  = false;

    bool   diagBaselineSet  = false;
    double diagBaselineE    = 0.0;
    double diagBaselinePmag = 0.0;
    double diagBaselineLmag = 0.0;

    /** @brief Snapshot of the selected body's fields shown in the Planet Info panel. */
    struct PlanetEditBuffer
    {
        char      name[128];
        float     mass;
        float     density;
        float     radius;
        glm::vec3 position;
        glm::vec3 velocity;
    } editBuffer;

    Actionbar actionbar_;

    void renderPlanetPopup(Window& window, Camera& camera,
                           const glm::mat4& view, const glm::mat4& projection,
                           std::vector<CelestialBody>& bodies);
    void renderPlanetInfo(CelestialBody& body);
    void renderPersistentBodyLabels(Window& window, Camera& camera,
                                    const glm::mat4& view, const glm::mat4& projection,
                                    const std::vector<CelestialBody>& bodies);
    void renderDiagnostics(const std::vector<CelestialBody>& bodies);
    void renderBodyContextMenu(std::vector<CelestialBody>& bodies, Camera& camera);

    /// Finalise the in-flight placement: build a CelestialBody from
    /// placementStartPosWU_ + the drag vector, append to @p bodies, select.
    void spawnPlacedBody(std::vector<CelestialBody>& bodies);
};
