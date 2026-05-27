#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

class Camera;
class PhysicsSystem;
class UIManager;
struct CelestialBody;

/**
 * @file SaveLoadModal.h
 * @brief Two centered popups for persistence: Save (filename input) and Load
 *        (one or two sections of saves, depending on platform).
 *
 * The Load modal looks different per build:
 *  - Desktop: a single recent-saves list rooted at `presets/`, plus a
 *    type-a-path input (matches the original behavior).
 *  - Web (`SOLARSYSTEM_BUILD_WEB`): two sections — "My saves" (writable,
 *    rooted at `/user/`) with per-row Export/Delete, and "Built-in presets"
 *    (read-only, rooted at `/presets/`) with Export. Plus an Import button
 *    in the header that opens the browser file picker.
 *
 * Overwrite confirmation in the Save popup is web-only — the desktop has
 * always silently overwritten existing files, and the plan preserves that.
 */
class SaveLoadModal
{
public:
    static constexpr const char* kSavePopupId = "Save Simulation";
    static constexpr const char* kLoadPopupId = "Load Simulation";

    /// Renders both popups (only one is open at a time). Call between NewFrame
    /// and Render.
    void render(std::vector<CelestialBody>& bodies, PhysicsSystem& physics, Camera& camera,
                UIManager& uiManager);

    /// Mark the Save popup as pending-open.
    void requestOpenSave()
    {
        pendingOpenSave_ = true;
    }

    /// Mark the Load popup as pending-open.
    void requestOpenLoad()
    {
        pendingOpenLoad_ = true;
    }

private:
    /// One save in the Load modal — either a user-writable file or a
    /// read-only shipped preset.
    struct SaveEntry
    {
        std::filesystem::path path;              ///< Full path used by SaveLoad I/O.
        std::string displayName;                 ///< Filename only, e.g. "trappist-1.txt".
        std::filesystem::file_time_type mtime{}; ///< Last-modified — used for sort on user saves.
        bool isBuiltIn = false;
    };

    /// Rescan both directories and rebuild @ref userSaves_ + @ref builtInPresets_.
    /// User saves are sorted newest-first; built-ins are sorted alphabetically.
    void gatherSaves();

    /// Pick an import filename that doesn't collide with an existing user save:
    /// `foo.txt` -> `foo-2.txt`, `foo-3.txt`, ... if needed. The returned name
    /// is the base filename only (no directory).
    std::string resolveImportName(const std::string& desired) const;

    /// Replace forbidden characters in @p name (slashes etc.) with underscores
    /// so it's safe to use as a file basename.
    static std::string sanitizeBasename(std::string name);

    bool pendingOpenSave_     = false;
    bool pendingOpenLoad_     = false;
    char filenameBuffer_[256] = "savefile.txt";

    std::vector<SaveEntry> userSaves_;
    std::vector<SaveEntry> builtInPresets_;

    /// Web-only confirm modals. The strings here are the target path / display
    /// name; the modal opens when the optional has a value.
    struct PendingPath
    {
        std::string path;
        std::string displayName;
    };
    std::optional<PendingPath> pendingOverwrite_; ///< Save: target already exists.
    std::optional<PendingPath> pendingDelete_;    ///< Load: user clicked delete on a user save.

    /// Set inside the overwrite-confirm popup so the Save popup beneath it
    /// closes on the next render frame — ImGui::CloseCurrentPopup only closes
    /// the popup we're currently inside, not the one stacked behind.
    bool closeSavePopupRequested_ = false;
};
