#include "SettingsModal.h"

#include "ui/UIManager.h"  // pulls glad before any GLFW header
#include "core/Camera.h"

#include <imgui.h>

void SettingsModal::render(Camera& camera, float& fieldOfView, float& guiScale,
                           UIManager& uiManager)
{
    if (pendingOpen_)
    {
        ImGui::OpenPopup(kPopupId);
        pendingOpen_ = false;
    }

    const ImVec2 viewportCenter = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(viewportCenter, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(360.0f, 0.0f), ImGuiCond_Appearing);

    if (!ImGui::BeginPopupModal(kPopupId, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        return;
    }

    float sensitivity = camera.getSensitivity();
    if (ImGui::SliderFloat("Mouse sensitivity", &sensitivity, 0.01f, 1.0f, "%.3f"))
    {
        camera.setSensitivity(sensitivity);
    }

    ImGui::SliderFloat("Field of view", &fieldOfView, 30.0f, 120.0f, "%.0f deg");

    if (ImGui::SliderFloat("GUI scale", &guiScale, 0.75f, 2.0f, "%.2fx"))
    {
        ImGui::GetIO().FontGlobalScale = guiScale;
    }

    if (ImGui::Checkbox("VSync", &uiManager.vsync))
    {
        uiManager.vsyncDirty = true;
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::Button("Close", ImVec2(120.0f, 0.0f)))
    {
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}
