#include "SaveLoadModal.h"

#include "ui/UIManager.h" // pulls glad before any GLFW header
#include "ui/UI.h"
#include "core/Camera.h"
#include "core/SaveLoad.h"
#include "core/WebPersistence.h"
#include "objects/CelestialBody.h"
#include "physics/PhysicsSystem.h"

#include <imgui.h>

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <system_error>

namespace fs = std::filesystem;

namespace
{
void invalidatePostLoadState(Camera& camera, UIManager& uiManager)
{
    camera.setMode(CameraMode::FREE);
    uiManager.clearSelection();
}

/// Wrapper around saveSimulation + syncToIDB that also surfaces a toast.
/// Returns whether the file was actually written.
bool performSave(const std::string& path, std::vector<CelestialBody>& bodies, PhysicsSystem& physics,
                 UIManager& uiManager)
{
    if (saveSimulation(path, bodies, physics))
    {
        web::syncToIDB();
        uiManager.toasts().success("Saved to " + path);
        return true;
    }
    uiManager.toasts().error("Save failed: cannot write " + path);
    return false;
}

/// Read the file at @p path and trigger a browser download (web). On
/// desktop this is a no-op — the file is already on the user's disk.
void exportEntry(const std::string& path, const std::string& displayName, UIManager& uiManager)
{
    std::ifstream in(path, std::ios::binary);
    if (!in)
    {
        uiManager.toasts().error("Export failed: cannot read " + path);
        return;
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    web::downloadBlob(displayName, ss.str());
    uiManager.toasts().info("Downloading " + displayName);
}
} // namespace

void SaveLoadModal::gatherSaves()
{
    userSaves_.clear();
    builtInPresets_.clear();

    // Scan @p directory for *.txt files and push them onto @p out tagged with
    // @p isBuiltIn. A missing directory is silently OK — on web, /user/ may
    // not exist yet until the first save lands.
    auto gatherFromDir = [](const std::string& directory, bool isBuiltIn, std::vector<SaveEntry>& out)
    {
        const fs::path dir(directory);
        std::error_code ec;
        if (!fs::exists(dir, ec) || !fs::is_directory(dir, ec))
        {
            return;
        }

        for (const auto& entry : fs::directory_iterator(dir, ec))
        {
            if (!entry.is_regular_file(ec))
                continue;
            if (entry.path().extension() != ".txt")
                continue;

            SaveEntry se;
            se.path        = entry.path();
            se.displayName = entry.path().filename().string();
            se.mtime       = fs::last_write_time(entry.path(), ec);
            se.isBuiltIn   = isBuiltIn;
            out.push_back(std::move(se));
        }
    };

    gatherFromDir(web::userSaveDir(), /*isBuiltIn=*/false, userSaves_);
    if (web::hasSeparateBuiltIns())
    {
        gatherFromDir(web::builtInPresetsDir(), /*isBuiltIn=*/true, builtInPresets_);
    }

    std::sort(userSaves_.begin(), userSaves_.end(),
              [](const SaveEntry& lhs, const SaveEntry& rhs) { return lhs.mtime > rhs.mtime; });

    std::sort(builtInPresets_.begin(), builtInPresets_.end(),
              [](const SaveEntry& lhs, const SaveEntry& rhs) { return lhs.displayName < rhs.displayName; });
}

std::string SaveLoadModal::sanitizeBasename(std::string name)
{
    for (char& c : name)
    {
        if (c == '/' || c == '\\' || c == ':' || c == '\0')
            c = '_';
    }
    if (name.empty())
        name = "imported.txt";
    return name;
}

std::string SaveLoadModal::resolveImportName(const std::string& desired) const
{
    auto exists = [&](const std::string& name)
    {
        const std::string fullPath = web::userSaveDir() + name;
        std::error_code ec;
        return fs::exists(fullPath, ec);
    };

    if (!exists(desired))
        return desired;

    // Split "foo.txt" -> stem "foo", ext ".txt"; otherwise append a suffix
    // directly to the full name.
    const fs::path p(desired);
    const std::string stem = p.stem().string();
    const std::string ext  = p.extension().string(); // ".txt" or ""

    for (int i = 2; i < 10000; ++i)
    {
        char buf[300];
        std::snprintf(buf, sizeof(buf), "%s-%d%s", stem.c_str(), i, ext.c_str());
        if (!exists(buf))
            return buf;
    }
    return desired; // give up after 10k collisions — caller will overwrite
}

void SaveLoadModal::render(std::vector<CelestialBody>& bodies, PhysicsSystem& physics, Camera& camera,
                           UIManager& uiManager)
{
    if (pendingOpenSave_)
    {
        ImGui::OpenPopup(kSavePopupId);
        pendingOpenSave_ = false;
    }
    if (pendingOpenLoad_)
    {
        gatherSaves();
        ImGui::OpenPopup(kLoadPopupId);
        pendingOpenLoad_ = false;
    }

    const ImVec2 viewportCenter = ImGui::GetMainViewport()->GetCenter();
    constexpr ImVec2 kButtonSize(120.0f, 0.0f);

    // ------------------------------------------------------------------
    // Save popup
    // ------------------------------------------------------------------
    ImGui::SetNextWindowPos(viewportCenter, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ui::modalSize(420.0f, 0.0f), ImGuiCond_Appearing);
    if (ImGui::BeginPopupModal(kSavePopupId, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (closeSavePopupRequested_)
        {
            closeSavePopupRequested_ = false;
            ImGui::CloseCurrentPopup();
        }

        const std::string hint = "File name (written under " + web::userSaveDir() + "):";
        ImGui::TextUnformatted(hint.c_str());
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputText("##savefilename", filenameBuffer_, sizeof(filenameBuffer_));

        ImGui::Spacing();
        if (ImGui::Button("Save", kButtonSize))
        {
#ifdef SOLARSYSTEM_BUILD_WEB
            const std::string sanitized = sanitizeBasename(filenameBuffer_);
            const std::string path      = web::userSaveDir() + sanitized;
            std::error_code ec;
            if (fs::exists(path, ec))
            {
                pendingOverwrite_ = PendingPath{path, sanitized};
                // Keep Save popup open; the overwrite confirm pops above it.
            }
            else
            {
                performSave(path, bodies, physics, uiManager);
                ImGui::CloseCurrentPopup();
            }
#else
            // Desktop preserves the original behavior: no sanitize, silent
            // overwrite of `presets/<filename>`.
            const std::string path = web::userSaveDir() + filenameBuffer_;
            performSave(path, bodies, physics, uiManager);
            ImGui::CloseCurrentPopup();
#endif
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", kButtonSize))
        {
            ImGui::CloseCurrentPopup();
        }

        // Overwrite confirm (web only — desktop falls through the !exists path).
        if (pendingOverwrite_.has_value())
        {
            ImGui::OpenPopup("##confirm-overwrite");
        }
        ImGui::SetNextWindowPos(viewportCenter, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal("##confirm-overwrite", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("'%s' already exists. Overwrite?", pendingOverwrite_->displayName.c_str());
            ImGui::Spacing();
            if (ImGui::Button("Overwrite", kButtonSize))
            {
                performSave(pendingOverwrite_->path, bodies, physics, uiManager);
                pendingOverwrite_.reset();
                closeSavePopupRequested_ = true; // drains on the next Save popup render
                ImGui::CloseCurrentPopup();      // closes the confirm now
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", kButtonSize))
            {
                pendingOverwrite_.reset();
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        ImGui::EndPopup();
    }

    // ------------------------------------------------------------------
    // Load popup
    // ------------------------------------------------------------------
    ImGui::SetNextWindowPos(viewportCenter, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ui::modalSize(520.0f, 400.0f), ImGuiCond_Appearing);
    if (ImGui::BeginPopupModal(kLoadPopupId, nullptr, 0))
    {
#ifdef SOLARSYSTEM_BUILD_WEB
        // ---- Web: two sections + Import / per-row Export / Delete ----
        if (ImGui::Button("Import .txt"))
        {
            std::string fname, contents;
            if (web::pickFile(fname, contents))
            {
                const std::string safe   = sanitizeBasename(fname);
                const std::string final_ = resolveImportName(safe);
                const std::string path   = web::userSaveDir() + final_;
                std::ofstream out(path, std::ios::binary);
                if (out)
                {
                    out.write(contents.data(), static_cast<std::streamsize>(contents.size()));
                    out.close();
                    web::syncToIDB();
                    uiManager.toasts().success("Imported " + final_);
                    gatherSaves();
                }
                else
                {
                    uiManager.toasts().error("Import failed: cannot write " + path);
                }
            }
        }
        ImGui::Separator();

        ImGui::BeginChild("##saveslist", ImVec2(0.0f, 280.0f), true);

        if (!userSaves_.empty())
        {
            ImGui::TextDisabled("My saves");
            ImGui::Separator();
            for (size_t i = 0; i < userSaves_.size(); ++i)
            {
                const SaveEntry& e = userSaves_[i];
                ImGui::PushID(static_cast<int>(i));

                if (ImGui::Selectable(e.displayName.c_str(), false, ImGuiSelectableFlags_AllowItemOverlap))
                {
                    const std::string full = e.path.string();
                    if (loadSimulation(full, bodies, physics))
                    {
                        invalidatePostLoadState(camera, uiManager);
                        uiManager.toasts().success("Loaded " + e.displayName);
                        ImGui::CloseCurrentPopup();
                    }
                    else
                    {
                        uiManager.toasts().error("Load failed: " + full);
                    }
                }

                // Right-aligned Export / Delete buttons.
                ImGui::SameLine(ImGui::GetContentRegionAvail().x - 130.0f);
                if (ImGui::SmallButton("Export"))
                {
                    exportEntry(e.path.string(), e.displayName, uiManager);
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("Delete"))
                {
                    pendingDelete_ = PendingPath{e.path.string(), e.displayName};
                }

                ImGui::PopID();
            }
            ImGui::Spacing();
        }

        ImGui::TextDisabled("Built-in presets");
        ImGui::Separator();
        for (size_t i = 0; i < builtInPresets_.size(); ++i)
        {
            const SaveEntry& e = builtInPresets_[i];
            ImGui::PushID(static_cast<int>(i + userSaves_.size()));

            if (ImGui::Selectable(e.displayName.c_str(), false, ImGuiSelectableFlags_AllowItemOverlap))
            {
                const std::string full = e.path.string();
                if (loadSimulation(full, bodies, physics))
                {
                    invalidatePostLoadState(camera, uiManager);
                    uiManager.toasts().success("Loaded " + e.displayName);
                    ImGui::CloseCurrentPopup();
                }
                else
                {
                    uiManager.toasts().error("Load failed: " + full);
                }
            }

            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 70.0f);
            if (ImGui::SmallButton("Export"))
            {
                exportEntry(e.path.string(), e.displayName, uiManager);
            }

            ImGui::PopID();
        }
        ImGui::EndChild();

        // Delete confirm.
        if (pendingDelete_.has_value())
        {
            ImGui::OpenPopup("##confirm-delete");
        }
        ImGui::SetNextWindowPos(viewportCenter, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal("##confirm-delete", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Delete '%s'? This cannot be undone.", pendingDelete_->displayName.c_str());
            ImGui::Spacing();
            if (ImGui::Button("Delete", kButtonSize))
            {
                std::error_code ec;
                fs::remove(pendingDelete_->path, ec);
                if (ec)
                {
                    uiManager.toasts().error("Delete failed: " + pendingDelete_->displayName);
                }
                else
                {
                    web::syncToIDB();
                    uiManager.toasts().info("Deleted " + pendingDelete_->displayName);
                    gatherSaves();
                }
                pendingDelete_.reset();
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", kButtonSize))
            {
                pendingDelete_.reset();
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        ImGui::Spacing();
        if (ImGui::Button("Close", kButtonSize))
        {
            ImGui::CloseCurrentPopup();
        }
#else
        // ---- Desktop: original single-list UX, unchanged ----
        ImGui::TextUnformatted("Recent saves (newest first):");
        ImGui::Spacing();

        if (userSaves_.empty())
        {
            const std::string emptyHint = "No save files found under " + web::userSaveDir() + ".";
            ImGui::TextDisabled("%s", emptyHint.c_str());
        }
        else
        {
            ImGui::BeginChild("##recentsaves", ImVec2(0.0f, 240.0f), true);
            for (const auto& e : userSaves_)
            {
                if (ImGui::Selectable(e.displayName.c_str()))
                {
                    const std::string full = e.path.string();
                    if (loadSimulation(full, bodies, physics))
                    {
                        invalidatePostLoadState(camera, uiManager);
                        uiManager.toasts().success("Loaded " + e.displayName);
                        ImGui::CloseCurrentPopup();
                    }
                    else
                    {
                        uiManager.toasts().error("Load failed: " + full);
                    }
                }
            }
            ImGui::EndChild();
        }

        ImGui::Spacing();
        ImGui::TextUnformatted("Or type a path:");
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputText("##loadfilename", filenameBuffer_, sizeof(filenameBuffer_));

        ImGui::Spacing();
        if (ImGui::Button("Load", kButtonSize))
        {
            const std::string path = web::userSaveDir() + filenameBuffer_;
            if (loadSimulation(path, bodies, physics))
            {
                invalidatePostLoadState(camera, uiManager);
                uiManager.toasts().success("Loaded " + path);
                ImGui::CloseCurrentPopup();
            }
            else
            {
                uiManager.toasts().error("Load failed: " + path);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", kButtonSize))
        {
            ImGui::CloseCurrentPopup();
        }
#endif
        ImGui::EndPopup();
    }
}
