#pragma once

/**
 * @file Tutorial.h
 * @brief Five-step first-run tutorial overlay.
 *
 * Walks the user through the core interactions: free-fly camera, body
 * selection + edit, drag-to-place, time controls, and view shortcuts.
 *
 * Auto-opens on first run (detected via the `.tutorial_seen` marker file
 * in the working directory); writes the marker on dismiss so subsequent
 * launches stay quiet. The Help button in the actionbar can re-open it.
 */
class Tutorial
{
public:
    /// ImGui popup identifier.
    static constexpr const char* kPopupId = "SolarSystemGL Tutorial";

    /// @return `true` if the marker file `.tutorial_seen` does NOT exist.
    static bool isFirstRun();

    /// Create the marker file so subsequent runs skip the auto-open.
    static void markSeen();

    /// Schedule the popup to open on the next #render call.
    void requestOpen()
    {
        pendingOpen_ = true;
        step_        = 0;
    }

    /// Draw the popup if open. Call between NewFrame and Render.
    void render();

private:
    bool pendingOpen_ = false;
    int step_         = 0;
};
