#include "SettingsModal.h"

#include "ui/UIManager.h" // pulls glad before any GLFW header
#include "ui/UI.h"
#include "core/Camera.h"
#include "core/Grid.h"

#include <imgui.h>

namespace
{
void renderGeneralSection(Camera& camera, float& fieldOfView, float& guiScale, UIManager& uiManager)
{
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
    ImGui::TextDisabled("Rendering quality");
    if (ImGui::Checkbox("MSAA 4x", &uiManager.msaaEnabled))
    {
        uiManager.msaaDirty = true;
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Multisample anti-aliasing. Disabling reduces GPU load — useful on mobile.");

    // Starfield layer tier. Ordered to match StarfieldQuality; selecting an
    // entry writes the enum straight through (no FBO rebuild — it's a uniform).
    const char* starfieldLabels[] = {"Off", "Low", "Medium", "High"};
    int starfieldIdx              = static_cast<int>(uiManager.starfieldQuality);
    if (ImGui::Combo("Starfield", &starfieldIdx, starfieldLabels, IM_ARRAYSIZE(starfieldLabels)))
    {
        uiManager.starfieldQuality = static_cast<StarfieldQuality>(starfieldIdx);
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Procedural night sky. High adds a faint dust layer, the Milky Way band, and "
                          "twinkle; lower tiers shade fewer layers — useful on mobile.");
}

void renderGridSection(GridSettings& grid)
{
    const char* styleLabels[] = {"Cartesian", "Radial"};
    int styleIdx              = static_cast<int>(grid.style);
    if (ImGui::Combo("Style", &styleIdx, styleLabels, IM_ARRAYSIZE(styleLabels)))
    {
        grid.style = static_cast<GridSettings::Style>(styleIdx);
    }

    ImGui::ColorEdit3("Base color", &grid.baseColor[0]);
    ImGui::SliderFloat("Opacity", &grid.opacity, 0.0f, 1.0f, "%.2f");

    ImGui::SliderFloat("Distortion strength", &grid.distortionStrength, 0.0001f, 0.5f, "%.4f",
                       ImGuiSliderFlags_Logarithmic);
    ImGui::SliderFloat("Falloff radius", &grid.falloffRadius, 0.1f, 20.0f, "%.2f");
    ImGui::SliderFloat("Max well depth", &grid.maxWellDepth, 1.0f, 200.0f, "%.1f");

    // Resolution presets — combo keeps the UI compact while still
    // exposing the three useful tiers. Custom values can be typed via
    // Ctrl+click on the slider below.
    struct ResolutionPreset
    {
        const char* label;
        int value;
    };
    static constexpr ResolutionPreset kResolutionPresets[] = {
        {"Low (100)", 100},
        {"Medium (200)", 200},
        {"High (400)", 400},
    };
    const char* resolutionPreview = "Custom";
    for (const auto& preset : kResolutionPresets)
    {
        if (preset.value == grid.resolution)
        {
            resolutionPreview = preset.label;
            break;
        }
    }
    if (ImGui::BeginCombo("Resolution", resolutionPreview))
    {
        for (const auto& preset : kResolutionPresets)
        {
            const bool isSelected = (preset.value == grid.resolution);
            if (ImGui::Selectable(preset.label, isSelected))
            {
                grid.resolution = preset.value;
            }
            if (isSelected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    ImGui::SliderFloat("Extent (WU)", &grid.extent, 2000.0f, 30000.0f, "%.0f");

    ImGui::Spacing();
    ImGui::TextDisabled("Visual polish");
    ImGui::ColorEdit3("Well color", &grid.wellColor[0]);
    ImGui::SliderInt("Major every", &grid.majorLineInterval, 1, 50);
    ImGui::SliderFloat("Major boost", &grid.majorLineBoost, 1.0f, 6.0f, "%.1fx");
    ImGui::SliderFloat("Fade start", &grid.distanceFadeStart, 0.0f, 20000.0f, "%.0f WU");
    ImGui::SliderFloat("Fade end", &grid.distanceFadeEnd, 0.0f, 30000.0f, "%.0f WU");

    ImGui::Spacing();
    ImGui::TextDisabled("Black hole flavor");
    ImGui::Checkbox("Schwarzschild falloff", &grid.useSchwarzschild);
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip(
            "Switch from Lorentzian (1 / (1 + r^2)) to Schwarzschild-style (1 / r) — peakier wells.");
    ImGui::Checkbox("Per-body color tint", &grid.perBodyTint);
    ImGui::SliderFloat("Singularity darken", &grid.singularityDarken, 0.0f, 2.0f, "%.2f");
}
} // namespace

void SettingsModal::render(Camera& camera, float& fieldOfView, float& guiScale, UIManager& uiManager,
                           GridSettings& gridSettings)
{
    if (pendingOpen_)
    {
        ImGui::OpenPopup(kPopupId);
        pendingOpen_ = false;
    }

    const ImVec2 viewportCenter = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(viewportCenter, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ui::modalSize(420.0f, 0.0f), ImGuiCond_Appearing);

    if (!ImGui::BeginPopupModal(kPopupId, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        return;
    }

    if (ImGui::CollapsingHeader("General", ImGuiTreeNodeFlags_DefaultOpen))
    {
        renderGeneralSection(camera, fieldOfView, guiScale, uiManager);
    }

    if (ImGui::CollapsingHeader("Grid", ImGuiTreeNodeFlags_DefaultOpen))
    {
        renderGridSection(gridSettings);
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
