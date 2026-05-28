#include "Actionbar.h"

#include "core/Window.h" // first: pulls glad before any GLFW/OpenGL header
#include "core/Camera.h"
#include "core/Grid.h"
#include "objects/CelestialBody.h"
#include "physics/PhysicsSystem.h"
#include "ui/UI.h"
#include "ui/UIManager.h"

#include <imgui.h>
#include <imgui_internal.h> // BeginViewportSideBar lives here

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace
{
constexpr float kSeparatorIndent = 8.0f;

constexpr ImGuiWindowFlags kBarFlags =
    ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

/// Vertical separator between actionbar groups.
void renderGroupSeparator()
{
    ImGui::SameLine(0.0f, kSeparatorIndent);
    ImGui::TextDisabled("|");
    ImGui::SameLine(0.0f, kSeparatorIndent);
}

/// Format simulated seconds since startup as "Day N HH:MM".
void formatSimulatedDate(double simulatedSeconds, char* out, size_t outSize)
{
    constexpr double kSecondsPerDay  = 86400.0;
    constexpr double kSecondsPerHour = 3600.0;
    constexpr double kSecondsPerMin  = 60.0;

    const double dayCount  = std::floor(simulatedSeconds / kSecondsPerDay);
    const double timeOfDay = simulatedSeconds - dayCount * kSecondsPerDay;
    const int hourOfDay    = static_cast<int>(std::floor(timeOfDay / kSecondsPerHour));
    const int minuteOfHour =
        static_cast<int>(std::floor((timeOfDay - hourOfDay * kSecondsPerHour) / kSecondsPerMin));

    std::snprintf(out, outSize, "Day %.0f %02d:%02d", dayCount, hourOfDay, minuteOfHour);
}

struct TimeScalePreset
{
    const char* label;
    float value;
};
constexpr TimeScalePreset kTimeScalePresets[] = {
    {"1k\xc3\x97", 1000.0f},    {"10k\xc3\x97", 10000.0f},  {"100k\xc3\x97", 100000.0f},
    {"1M\xc3\x97", 1000000.0f}, {"5M\xc3\x97", 5000000.0f},
};
} // namespace

void Actionbar::renderTopBar(Window& window, Camera& camera, PhysicsSystem& physics,
                             std::vector<CelestialBody>& bodies, UIManager& uiManager,
                             GridSettings& gridSettings, float deltaTime)
{
    (void) window; // size is taken from the main viewport directly

    const float instantFps = 1.0f / std::max(deltaTime, 1e-6f);
    smoothedFps_           = smoothedFps_ * 0.9f + instantFps * 0.1f;

    if (!ImGui::BeginViewportSideBar("##actionbar_top", ImGui::GetMainViewport(), ImGuiDir_Up, kTopBarHeight,
                                     kBarFlags))
    {
        ImGui::End();
        return;
    }

    // Actionbar reflow uses the "compact" threshold (~960px) — the full
    // desktop layout doesn't fit below that, so the secondary groups move
    // into a "..." overflow menu and the right-anchored FPS readout is
    // dropped (it collides with the buttons once the font scales up).
    const bool mobile = ui::isCompactViewport();

    // --- Group 1: Menu + System dropdown ---------------------------------
    if (ImGui::Button("Menu"))
    {
        uiManager.menuRequested = true;
    }
    ImGui::SetItemTooltip("Return to the main menu (Esc)");
    ImGui::SameLine();

    const int selectedIdx     = uiManager.getSelectedPlanetIndex();
    const char* systemPreview = (selectedIdx >= 0 && selectedIdx < static_cast<int>(bodies.size()))
                                    ? bodies[selectedIdx].name.c_str()
                                    : "System";
    ImGui::SetNextItemWidth(mobile ? 110.0f : 160.0f);
    if (ImGui::BeginCombo("##system", systemPreview))
    {
        for (size_t i = 0; i < bodies.size(); ++i)
        {
            const bool isSelected   = (static_cast<int>(i) == selectedIdx);
            const BodyType type     = bodies[i].classify();
            const glm::vec3 chipRgb = bodyTypeColor(type);
            const ImVec4 chipColor(chipRgb.r, chipRgb.g, chipRgb.b, 1.0f);

            ImGui::PushID(static_cast<int>(i));
            if (ImGui::Selectable("##row", isSelected, ImGuiSelectableFlags_AllowOverlap,
                                  ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing())))
            {
                uiManager.setSelected(static_cast<int>(i));
            }
            ImGui::SameLine();
            ImGui::TextColored(chipColor, "%-9s", bodyTypeLabel(type));
            ImGui::SameLine();
            ImGui::TextUnformatted(bodies[i].name.c_str());
            if (isSelected)
                ImGui::SetItemDefaultFocus();
            ImGui::PopID();
        }
        ImGui::EndCombo();
    }
    ImGui::SetItemTooltip("Select a body");
    ImGui::SameLine();
    if (ImGui::Button("+"))
    {
        uiManager.addPlanetRequested = true;
    }
    ImGui::SetItemTooltip("Add a new planet");
    renderGroupSeparator();

    const CameraMode currentMode = camera.getMode();
    const bool hasSelection      = (selectedIdx >= 0 && selectedIdx < static_cast<int>(bodies.size()));

    // Free / Orbital rendered as a segmented control rather than ImGui
    // radio buttons: the radio's tiny outline reads as invisible against
    // the dark actionbar, so users couldn't tell which mode was active.
    // Two pill-buttons with a filled-blue selected state are unambiguous.
    auto drawSegmentedToggle = [](const char* label, bool selected, bool disabled) -> bool
    {
        const ImVec4 activeFill(0.22f, 0.44f, 0.78f, 1.00f);
        const ImVec4 activeHover(0.28f, 0.52f, 0.88f, 1.00f);
        const ImVec4 inactiveFill(0.07f, 0.11f, 0.18f, 0.85f);
        const ImVec4 inactiveHover(0.14f, 0.22f, 0.34f, 1.00f);

        ImGui::PushStyleColor(ImGuiCol_Button, selected ? activeFill : inactiveFill);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, selected ? activeHover : inactiveHover);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, activeFill);
        ImGui::BeginDisabled(disabled);
        const bool clicked = ImGui::Button(label);
        ImGui::EndDisabled();
        ImGui::PopStyleColor(3);
        return clicked;
    };

    struct ViewPresetOption
    {
        const char* label;
        const char* shortcut;
        int value;
    };
    static constexpr ViewPresetOption kViewPresets[] = {
        {"Default", "F1", 0},
        {"Top-down", "F2", 1},
        {"Side-on", "F3", 2},
    };

    if (!mobile)
    {
        // ========================== Desktop layout ==========================

        // --- Group 2: Time controls --------------------------------------
        ImGui::Checkbox("Paused", &physics.paused);
        ImGui::SetItemTooltip("Pause / resume the simulation (Space)");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(140.0f);
        ImGui::SliderFloat("##timescale", &physics.timeScale, 1000.0f, 5.0e6f, "%.0fx",
                           ImGuiSliderFlags_Logarithmic);
        ImGui::SetItemTooltip("Simulated seconds per real second (log scale)");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(72.0f);
        if (ImGui::BeginCombo("##timepreset", "Preset", ImGuiComboFlags_NoArrowButton))
        {
            for (const auto& preset : kTimeScalePresets)
            {
                if (ImGui::Selectable(preset.label))
                {
                    physics.timeScale = preset.value;
                }
            }
            ImGui::EndCombo();
        }
        ImGui::SetItemTooltip("Time-scale presets");
        ImGui::SameLine();

        char simDateBuffer[32];
        formatSimulatedDate(physics.getSimulatedTimeSeconds(), simDateBuffer, sizeof(simDateBuffer));
        ImGui::TextDisabled("%s", simDateBuffer);
        ImGui::SetItemTooltip("Simulated time elapsed since Start Simulation");

        // The date text width grows / shrinks as digits roll over (e.g. "Day 9"
        // vs "Day 92") and Inter is a proportional font, so without padding the
        // controls to the right would jitter. Reserve a fixed slot by padding
        // a Dummy out to the worst-case width of "Day 9999 23:59" so the next
        // group always starts at the same X regardless of the current date.
        const float widestDateWidth  = ImGui::CalcTextSize("Day 9999 23:59").x + 10.0f;
        const float currentDateWidth = ImGui::GetItemRectSize().x;
        if (currentDateWidth < widestDateWidth)
        {
            ImGui::SameLine();
            ImGui::Dummy(ImVec2(widestDateWidth - currentDateWidth, 0.0f));
        }

        renderGroupSeparator();

        // --- Group 3: Camera controls ------------------------------------
        if (drawSegmentedToggle("Free", currentMode == CameraMode::FREE, /*disabled=*/false))
        {
            camera.setMode(CameraMode::FREE);
        }
        ImGui::SetItemTooltip("Free-fly camera (WASD + right-click drag) [1]");
        ImGui::SameLine();
        if (drawSegmentedToggle("Orbital", currentMode == CameraMode::ORBITAL, /*disabled=*/!hasSelection))
        {
            camera.flyToOrbital(selectedIdx, bodies[selectedIdx].renderPosition(),
                                bodies[selectedIdx].focusDistance());
        }
        ImGui::SetItemTooltip("Orbit the selected body [2]");
        ImGui::SameLine();

        ImGui::BeginDisabled(!hasSelection);
        if (ImGui::Button("Focus") && hasSelection)
        {
            camera.flyToOrbital(selectedIdx, bodies[selectedIdx].renderPosition(),
                                bodies[selectedIdx].focusDistance());
        }
        ImGui::SetItemTooltip("Fly to the selected body (F)");
        ImGui::EndDisabled();
        ImGui::SameLine();

        ImGui::SetNextItemWidth(72.0f);
        if (ImGui::BeginCombo("##view", "View", ImGuiComboFlags_NoArrowButton))
        {
            for (const auto& v : kViewPresets)
            {
                if (ImGui::Selectable(v.label, false, ImGuiSelectableFlags_None))
                {
                    uiManager.viewPresetRequested = v.value;
                }
                ImGui::SameLine(120.0f);
                ImGui::TextDisabled("%s", v.shortcut);
            }
            ImGui::EndCombo();
        }
        ImGui::SetItemTooltip("Fly to a preset view  (F1 / F2 / F3 — R for instant reset)");
        renderGroupSeparator();

        // --- Group 4: Settings / Save / Load -----------------------------
        if (ImGui::Button("Settings"))
            uiManager.settingsRequested = true;
        ImGui::SetItemTooltip("Sensitivity, FOV, GUI scale, VSync");
        ImGui::SameLine();
        if (ImGui::Button("Save"))
            uiManager.saveRequested = true;
        ImGui::SetItemTooltip("Write the current state to a save file");
        ImGui::SameLine();
        if (ImGui::Button("Load"))
            uiManager.loadRequested = true;
        ImGui::SetItemTooltip("Load a previously saved state");
        ImGui::SameLine();
        if (ImGui::Button("?"))
            uiManager.helpRequested = true;
        ImGui::SetItemTooltip("Re-open the tutorial");
    }
    else
    {
        // =========================== Mobile layout ===========================
        // Only the "⋯" trigger fits beside Menu/System/+ at phone widths once
        // the font scales up — everything else lives in the overflow popup.
        // Plain ASCII "More..." — the Inter atlas is loaded with the default
        // Latin range only, so non-ASCII glyphs (U+22EF, U+2026) would render
        // as missing-glyph boxes.
        if (ImGui::Button("More..."))
        {
            ImGui::OpenPopup("##overflow");
        }
        ImGui::SetItemTooltip("More controls");

        const ImVec2 popupPos(ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y + 6.0f);
        ImGui::SetNextWindowPos(popupPos, ImGuiCond_Appearing);
        ImGui::SetNextWindowSize(ImVec2(280.0f, 0.0f), ImGuiCond_Appearing);
        if (ImGui::BeginPopup("##overflow"))
        {
            ImGui::TextDisabled("Time");
            ImGui::Checkbox("Paused", &physics.paused);
            ImGui::SetNextItemWidth(-1.0f);
            ImGui::SliderFloat("##timescale_overflow", &physics.timeScale, 1000.0f, 5.0e6f, "%.0fx",
                               ImGuiSliderFlags_Logarithmic);
            for (size_t i = 0; i < IM_ARRAYSIZE(kTimeScalePresets); ++i)
            {
                if (ImGui::SmallButton(kTimeScalePresets[i].label))
                {
                    physics.timeScale = kTimeScalePresets[i].value;
                }
                if (i + 1 < IM_ARRAYSIZE(kTimeScalePresets))
                    ImGui::SameLine();
            }

            ImGui::Separator();
            ImGui::TextDisabled("Camera");
            if (drawSegmentedToggle("Free", currentMode == CameraMode::FREE, /*disabled=*/false))
            {
                camera.setMode(CameraMode::FREE);
            }
            ImGui::SameLine();
            if (drawSegmentedToggle("Orbital", currentMode == CameraMode::ORBITAL,
                                    /*disabled=*/!hasSelection))
            {
                camera.flyToOrbital(selectedIdx, bodies[selectedIdx].renderPosition(),
                                    bodies[selectedIdx].focusDistance());
            }
            ImGui::BeginDisabled(!hasSelection);
            if (ImGui::Button("Focus", ImVec2(-1, 0)) && hasSelection)
            {
                camera.flyToOrbital(selectedIdx, bodies[selectedIdx].renderPosition(),
                                    bodies[selectedIdx].focusDistance());
            }
            ImGui::EndDisabled();
            for (const auto& v : kViewPresets)
            {
                char label[32];
                std::snprintf(label, sizeof(label), "View: %s", v.label);
                if (ImGui::Button(label, ImVec2(-1, 0)))
                {
                    uiManager.viewPresetRequested = v.value;
                    ImGui::CloseCurrentPopup();
                }
            }

            ImGui::Separator();
            ImGui::TextDisabled("File");
            if (ImGui::Button("Save", ImVec2(-1, 0)))
            {
                uiManager.saveRequested = true;
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::Button("Load", ImVec2(-1, 0)))
            {
                uiManager.loadRequested = true;
                ImGui::CloseCurrentPopup();
            }

            ImGui::Separator();
            ImGui::TextDisabled("Tools");
            if (ImGui::Button("Settings", ImVec2(-1, 0)))
            {
                uiManager.settingsRequested = true;
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::Button("Help", ImVec2(-1, 0)))
            {
                uiManager.helpRequested = true;
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::Button(uiManager.demoModeActive ? "Exit Demo Mode" : "Demo Mode", ImVec2(-1, 0)))
            {
                uiManager.demoModeActive = !uiManager.demoModeActive;
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::Button("Screenshot", ImVec2(-1, 0)))
            {
                uiManager.screenshotRequested = true;
                ImGui::CloseCurrentPopup();
            }

            // Visual toggles — the bottom action bar is hidden on compact
            // viewports (it doesn't fit), so its checkboxes live here instead.
            ImGui::Separator();
            ImGui::TextDisabled("Show");
            ImGui::Checkbox("Trails", &uiManager.showTrails);
            ImGui::Checkbox("Labels", &uiManager.showBodyLabels);
            ImGui::Checkbox("Bloom", &uiManager.showBloom);
            ImGui::Checkbox("Grid", &gridSettings.visible);
            ImGui::Checkbox("Diagnostics", &uiManager.showDiagnostics);
            if (ImGui::Checkbox("VSync", &uiManager.vsync))
            {
                uiManager.vsyncDirty = true;
            }

            ImGui::EndPopup();
        }
    }

    // --- Group 5: FPS (right-anchored) — desktop only; on compact viewports
    // the buttons would otherwise collide with the right-anchored readout.
    if (!mobile)
    {
        const float windowWidth  = ImGui::GetWindowSize().x;
        const float fpsTextWidth = ImGui::CalcTextSize("FPS: 9999.9").x;
        ImGui::SameLine(windowWidth - fpsTextWidth - 12.0f);
        ImGui::Text("FPS: %.1f", smoothedFps_);
        ImGui::SetItemTooltip("Frames per second (smoothed)");
    }

    ImGui::End();
}

void Actionbar::renderBottomBar(Window& /*window*/, UIManager& uiManager, GridSettings& gridSettings)
{
    // On compact viewports the six toggles don't fit on one row; they're
    // surfaced in the top bar's "More..." overflow instead, so skip the
    // bottom bar entirely and give the canvas the extra height.
    if (ui::isCompactViewport())
    {
        return;
    }

    if (!ImGui::BeginViewportSideBar("##actionbar_bottom", ImGui::GetMainViewport(), ImGuiDir_Down,
                                     kBottomBarHeight, kBarFlags))
    {
        ImGui::End();
        return;
    }

    // Bottom bar holds only system-wide toggles. Per-body visualisations
    // (Prediction, Lagrange) live inside the Planet Info panel since they
    // require a selection to mean anything.
    ImGui::Checkbox("Trails", &uiManager.showTrails);
    ImGui::SetItemTooltip("Render orbit trails as line strips");
    ImGui::SameLine();
    ImGui::Checkbox("Labels", &uiManager.showBodyLabels);
    ImGui::SetItemTooltip("Persistent name labels above every body (off by default; hover always works)");
    ImGui::SameLine();
    ImGui::Checkbox("Bloom", &uiManager.showBloom);
    ImGui::SetItemTooltip("Render emissive bloom for stars");
    ImGui::SameLine();
    ImGui::Checkbox("Grid", &gridSettings.visible);
    ImGui::SetItemTooltip("Render the spacetime distortion grid");
    ImGui::SameLine();
    ImGui::Checkbox("Diagnostics", &uiManager.showDiagnostics);
    ImGui::SetItemTooltip("Show the conservation-of-energy diagnostics panel");
    ImGui::SameLine();
    if (ImGui::Checkbox("VSync", &uiManager.vsync))
    {
        uiManager.vsyncDirty = true;
    }
    ImGui::SetItemTooltip("Cap frame rate to the monitor refresh");

    ImGui::End();
}
