#pragma once

class Camera;
class UIManager;
struct GridSettings;

/**
 * @file SettingsModal.h
 * @brief Centered "Settings" popup — sensitivity, FOV, GUI scale, VSync, Grid.
 */

/**
 * @brief Renders the settings popup. Stateless — owner holds the open flag.
 */
class SettingsModal
{
public:
    /// Popup identifier used by ImGui::OpenPopup / BeginPopupModal.
    static constexpr const char* kPopupId = "Settings";

    /**
     * @brief Draw the settings popup. Must run between NewFrame and Render.
     *
     * Call `requestOpen()` (sets the local flag) in the same frame the user
     * clicks the Settings button — the next #render call will issue
     * `ImGui::OpenPopup` and the modal stays alive until the user dismisses
     * it.
     *
     * @param camera       Read/write — sensitivity getter/setter.
     * @param fieldOfView  Read/write — vertical FOV in degrees.
     * @param guiScale     Read/write — applied to ImGuiIO::FontGlobalScale.
     * @param uiManager    Read/write — VSync state (and vsyncDirty flag).
     * @param gridSettings Read/write — full grid configuration (visual + physics + geometry).
     */
    void render(Camera& camera, float& fieldOfView, float& guiScale, UIManager& uiManager,
                GridSettings& gridSettings);

    /// Mark the popup as pending-open. The next #render call will issue OpenPopup.
    void requestOpen()
    {
        pendingOpen_ = true;
    }

private:
    bool pendingOpen_ = false;
};
