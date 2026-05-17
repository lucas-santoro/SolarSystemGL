#include "Actionbar.h"

#include "core/Window.h"   // first: pulls glad before any GLFW/OpenGL header
#include "core/Camera.h"
#include "objects/CelestialBody.h"
#include "physics/PhysicsSystem.h"
#include "ui/UIManager.h"

#include <imgui.h>
#include <imgui_internal.h>  // BeginViewportSideBar lives here

#include <algorithm>

namespace
{
    constexpr float kSeparatorIndent = 8.0f;

    constexpr ImGuiWindowFlags kBarFlags =
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoScrollbar     |
        ImGuiWindowFlags_NoScrollWithMouse;

    /// Vertical separator between actionbar groups.
    void renderGroupSeparator()
    {
        ImGui::SameLine(0.0f, kSeparatorIndent);
        ImGui::TextDisabled("|");
        ImGui::SameLine(0.0f, kSeparatorIndent);
    }
}

void Actionbar::renderTopBar(Window& window, Camera& camera, PhysicsSystem& physics,
                             std::vector<CelestialBody>& bodies, UIManager& uiManager,
                             float deltaTime)
{
    (void)window;  // size is taken from the main viewport directly

    const float instantFps = 1.0f / std::max(deltaTime, 1e-6f);
    smoothedFps_ = smoothedFps_ * 0.9f + instantFps * 0.1f;

    if (!ImGui::BeginViewportSideBar("##actionbar_top",
                                     ImGui::GetMainViewport(),
                                     ImGuiDir_Up,
                                     kTopBarHeight,
                                     kBarFlags))
    {
        ImGui::End();
        return;
    }

    // --- Group 1: Menu + System dropdown ---------------------------------
    if (ImGui::Button("Menu"))
    {
        uiManager.menuRequested = true;
    }
    ImGui::SameLine();

    const int  selectedIdx     = uiManager.getSelectedPlanetIndex();
    const char* systemPreview  = (selectedIdx >= 0 && selectedIdx < static_cast<int>(bodies.size()))
                                     ? bodies[selectedIdx].name.c_str()
                                     : "System";
    ImGui::SetNextItemWidth(140.0f);
    if (ImGui::BeginCombo("##system", systemPreview))
    {
        for (size_t i = 0; i < bodies.size(); ++i)
        {
            const bool isSelected = (static_cast<int>(i) == selectedIdx);
            if (ImGui::Selectable(bodies[i].name.c_str(), isSelected))
            {
                uiManager.setSelected(static_cast<int>(i));
            }
            if (isSelected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    ImGui::SameLine();
    if (ImGui::Button("+"))
    {
        uiManager.addPlanetRequested = true;
    }
    renderGroupSeparator();

    // --- Group 2: Time controls ------------------------------------------
    if (ImGui::Checkbox("Paused", &physics.paused)) {}
    ImGui::SameLine();
    ImGui::SetNextItemWidth(180.0f);
    ImGui::SliderFloat("##timescale", &physics.timeScale,
                       1.0f, 5.0e6f, "%.0fx", ImGuiSliderFlags_Logarithmic);
    ImGui::SameLine();
    if (ImGui::SmallButton("1x"))     physics.timeScale = 1.0f;
    ImGui::SameLine();
    if (ImGui::SmallButton("1k"))     physics.timeScale = 1000.0f;
    ImGui::SameLine();
    if (ImGui::SmallButton("100k"))   physics.timeScale = 100000.0f;
    ImGui::SameLine();
    if (ImGui::SmallButton("1M"))     physics.timeScale = 1.0e6f;
    renderGroupSeparator();

    // --- Group 3: Camera controls ----------------------------------------
    const CameraMode currentMode = camera.getMode();
    const bool       hasSelection = (selectedIdx >= 0
                                     && selectedIdx < static_cast<int>(bodies.size()));

    if (ImGui::RadioButton("Free", currentMode == CameraMode::FREE))
    {
        camera.setMode(CameraMode::FREE);
    }
    ImGui::SameLine();
    ImGui::BeginDisabled(!hasSelection);
    if (ImGui::RadioButton("Orbital", currentMode == CameraMode::ORBITAL))
    {
        if (hasSelection)
        {
            camera.flyToOrbital(selectedIdx,
                                bodies[selectedIdx].renderPosition(),
                                bodies[selectedIdx].focusDistance());
        }
    }
    ImGui::EndDisabled();
    ImGui::SameLine();

    ImGui::BeginDisabled(!hasSelection);
    if (ImGui::Button("Focus") && hasSelection)
    {
        camera.flyToOrbital(selectedIdx,
                            bodies[selectedIdx].renderPosition(),
                            bodies[selectedIdx].focusDistance());
    }
    ImGui::EndDisabled();
    ImGui::SameLine();

    if (ImGui::Button("Reset Cam"))
    {
        camera.reset();
    }
    renderGroupSeparator();

    // --- Group 4: Settings / Save / Load ---------------------------------
    if (ImGui::Button("Settings")) uiManager.settingsRequested = true;
    ImGui::SameLine();
    if (ImGui::Button("Save"))     uiManager.saveRequested     = true;
    ImGui::SameLine();
    if (ImGui::Button("Load"))     uiManager.loadRequested     = true;

    // --- Group 5: FPS (right-anchored) -----------------------------------
    const float windowWidth = ImGui::GetWindowSize().x;
    const float fpsTextWidth = ImGui::CalcTextSize("FPS: 9999.9").x;
    ImGui::SameLine(windowWidth - fpsTextWidth - 12.0f);
    ImGui::Text("FPS: %.1f", smoothedFps_);

    ImGui::End();
}

void Actionbar::renderBottomBar(Window& /*window*/, UIManager& uiManager)
{
    if (!ImGui::BeginViewportSideBar("##actionbar_bottom",
                                     ImGui::GetMainViewport(),
                                     ImGuiDir_Down,
                                     kBottomBarHeight,
                                     kBarFlags))
    {
        ImGui::End();
        return;
    }

    ImGui::Checkbox("Trails",      &uiManager.showTrails);
    ImGui::SameLine();
    ImGui::Checkbox("Bloom",       &uiManager.showBloom);
    ImGui::SameLine();
    ImGui::Checkbox("Diagnostics", &uiManager.showDiagnostics);
    ImGui::SameLine();
    if (ImGui::Checkbox("VSync", &uiManager.vsync))
    {
        uiManager.vsyncDirty = true;
    }

    ImGui::End();
}
