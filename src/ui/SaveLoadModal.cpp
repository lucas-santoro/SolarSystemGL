#include "SaveLoadModal.h"

#include "ui/UIManager.h"  // pulls glad before any GLFW header
#include "core/Camera.h"
#include "core/SaveLoad.h"
#include "objects/CelestialBody.h"
#include "physics/PhysicsSystem.h"

#include <imgui.h>

#include <algorithm>
#include <system_error>

namespace fs = std::filesystem;

namespace
{
    constexpr const char* kSavesDirectory = "presets";

    void invalidatePostLoadState(Camera& camera, UIManager& uiManager)
    {
        camera.setMode(CameraMode::FREE);
        uiManager.clearSelection();
    }
}

void SaveLoadModal::refreshRecentSaves()
{
    recentSaves_.clear();

    const fs::path directory(kSavesDirectory);
    std::error_code errorCode;
    if (!fs::exists(directory, errorCode) || !fs::is_directory(directory, errorCode))
    {
        return;
    }

    for (const auto& entry : fs::directory_iterator(directory, errorCode))
    {
        if (entry.is_regular_file(errorCode) && entry.path().extension() == ".txt")
        {
            recentSaves_.push_back(entry.path());
        }
    }

    std::sort(recentSaves_.begin(), recentSaves_.end(),
              [](const fs::path& lhs, const fs::path& rhs)
              {
                  std::error_code ignored;
                  const auto lhsTime = fs::last_write_time(lhs, ignored);
                  const auto rhsTime = fs::last_write_time(rhs, ignored);
                  return lhsTime > rhsTime;
              });
}

void SaveLoadModal::render(std::vector<CelestialBody>& bodies, PhysicsSystem& physics,
                           Camera& camera, UIManager& uiManager)
{
    if (pendingOpenSave_)
    {
        ImGui::OpenPopup(kSavePopupId);
        pendingOpenSave_ = false;
    }
    if (pendingOpenLoad_)
    {
        refreshRecentSaves();
        ImGui::OpenPopup(kLoadPopupId);
        pendingOpenLoad_ = false;
    }

    const ImVec2 viewportCenter = ImGui::GetMainViewport()->GetCenter();
    constexpr ImVec2 kButtonSize(120.0f, 0.0f);

    // ---- Save popup ----
    ImGui::SetNextWindowPos(viewportCenter, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(420.0f, 0.0f), ImGuiCond_Appearing);
    if (ImGui::BeginPopupModal(kSavePopupId, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextUnformatted("File name (written under presets/):");
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputText("##savefilename", filenameBuffer_, sizeof(filenameBuffer_));

        ImGui::Spacing();
        if (ImGui::Button("Save", kButtonSize))
        {
            const std::string path = std::string(kSavesDirectory) + "/" + filenameBuffer_;
            if (saveSimulation(path, bodies, physics))
            {
                uiManager.toasts().success("Saved to " + path);
            }
            else
            {
                uiManager.toasts().error("Save failed: cannot write " + path);
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", kButtonSize))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // ---- Load popup ----
    ImGui::SetNextWindowPos(viewportCenter, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(480.0f, 360.0f), ImGuiCond_Appearing);
    if (ImGui::BeginPopupModal(kLoadPopupId, nullptr, 0))
    {
        ImGui::TextUnformatted("Recent saves (newest first):");
        ImGui::Spacing();

        if (recentSaves_.empty())
        {
            ImGui::TextDisabled("No save files found under presets/.");
        }
        else
        {
            ImGui::BeginChild("##recentsaves", ImVec2(0.0f, 240.0f), true);
            for (const auto& path : recentSaves_)
            {
                const std::string displayName = path.filename().string();
                if (ImGui::Selectable(displayName.c_str()))
                {
                    const std::string fullPath = path.string();
                    if (loadSimulation(fullPath, bodies, physics))
                    {
                        invalidatePostLoadState(camera, uiManager);
                        uiManager.toasts().success("Loaded " + displayName);
                        ImGui::CloseCurrentPopup();
                    }
                    else
                    {
                        uiManager.toasts().error("Load failed: " + fullPath);
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
            const std::string path = std::string(kSavesDirectory) + "/" + filenameBuffer_;
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
        ImGui::EndPopup();
    }
}
