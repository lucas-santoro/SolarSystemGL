#pragma once

#include <filesystem>
#include <string>
#include <vector>

class Camera;
class PhysicsSystem;
class UIManager;
struct CelestialBody;

/**
 * @file SaveLoadModal.h
 * @brief Two centered popups for persistence: Save (filename input) and Load
 *        (recent-saves table).
 */

/**
 * @brief Save and Load popups backed by `saveSimulation` / `loadSimulation`.
 *
 * The modal is stateful only in the small ways it needs to be: a filename
 * input buffer, a cached list of recent save files, and two open-request
 * flags toggled by the actionbar buttons.
 */
class SaveLoadModal
{
public:
    static constexpr const char* kSavePopupId = "Save Simulation";
    static constexpr const char* kLoadPopupId = "Load Simulation";

    /// Renders both popups (only one is open at a time). Call between NewFrame
    /// and Render.
    void render(std::vector<CelestialBody>& bodies, PhysicsSystem& physics,
                Camera& camera, UIManager& uiManager);

    /// Mark the Save popup as pending-open.
    void requestOpenSave() { pendingOpenSave_ = true; }

    /// Mark the Load popup as pending-open.
    void requestOpenLoad() { pendingOpenLoad_ = true; }

private:
    /// Re-scan the presets directory for *.txt files and cache them ordered
    /// by modification time (newest first).
    void refreshRecentSaves();

    bool pendingOpenSave_ = false;
    bool pendingOpenLoad_ = false;
    char filenameBuffer_[256] = "savefile.txt";

    std::vector<std::filesystem::path> recentSaves_;
};
