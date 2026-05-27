#include "Tutorial.h"

#include "ui/UI.h"
#include "core/WebPersistence.h"

#include <imgui.h>

#include <filesystem>
#include <fstream>

namespace
{
// Where the "tutorial already seen" marker lives. On web it must sit under
// the IDBFS mount (/user) so it survives a reload — the MEMFS root does
// not persist. On desktop it stays in the working directory.
std::string seenMarkerPath()
{
#ifdef SOLARSYSTEM_BUILD_WEB
    return web::userSaveDir() + ".tutorial_seen";
#else
    return ".tutorial_seen";
#endif
}
} // namespace

namespace
{
struct Step
{
    const char* title;
    const char* body;
};

constexpr Step kSteps[] = {
    {"Welcome to SolarSystemGL", "A Newtonian N-body simulator of our solar system, with realistic\n"
                                 "gravity, atmospheres, ring shadows, and Lagrange points.\n\n"
                                 "This quick tour explains the main controls. You can re-open it\n"
                                 "anytime via the \"?\" button on the right of the top action bar."},
    {"Camera", "RIGHT-CLICK + DRAG  rotate the camera (mouse-look in Free mode,\n"
               "                    orbit around the selected body in Orbital).\n"
               "WASD                fly the camera through space (Free mode only).\n"
               "MOUSE WHEEL         zoom in/out (Orbital) or change speed (Free).\n"
               "R                   reset camera to the default cinematic angle.\n"
               "F1 / F2 / F3        Default / Top-down / Side-on view presets.\n"
               "F5–F8               4 camera bookmarks (Shift+key = save slot)."},
    {"Bodies & edit", "LEFT-CLICK a body to select it. The Planet Info panel drops down\n"
                      "on the right with editable Name, Mass, Density, Position, and\n"
                      "Velocity. Press Apply to commit changes.\n\n"
                      "RIGHT-CLICK a body for a context menu: Focus, Edit, Make Star,\n"
                      "Duplicate, Remove.\n\n"
                      "Per-body visualisations (Path prediction, Lagrange points) live\n"
                      "inside the Planet Info panel."},
    {"Drag-to-place", "LEFT-CLICK + DRAG in empty space to spawn a new Earth-mass body\n"
                      "where you clicked. The drag direction and length set the initial\n"
                      "velocity. A short drag falls into the Sun; a long drag escapes.\n\n"
                      "Most drags produce elliptical / hyperbolic trajectories — that's\n"
                      "real Kepler physics, not a bug. Use the \"+\" button in the top\n"
                      "bar to spawn with the form-based dialog instead."},
    {"Time & saving", "TIME SCALE slider in the top bar controls simulated seconds per\n"
                      "real second. Range 1k× to 5M× (log).  SPACE to pause.\n\n"
                      "BOTTOM BAR toggles Trails, Bloom, Grid, Diagnostics, VSync, and\n"
                      "Labels globally.\n\n"
                      "SAVE / LOAD persist the whole simulation to .txt files under\n"
                      "presets/. The Load modal lists recent saves sorted by mtime.\n\n"
                      "F12  screenshot to screenshots/."},
};

constexpr int kStepCount = static_cast<int>(sizeof(kSteps) / sizeof(kSteps[0]));
} // namespace

bool Tutorial::isFirstRun()
{
    std::error_code ec;
    return !std::filesystem::exists(seenMarkerPath(), ec);
}

void Tutorial::markSeen()
{
    std::ofstream out(seenMarkerPath());
    if (out)
        out << "done\n";
    out.close();
    // Persist to IndexedDB on web (no-op on desktop) so the marker — and any
    // pending user saves — survive the next reload.
    web::syncToIDB();
}

void Tutorial::render()
{
    if (pendingOpen_)
    {
        ImGui::OpenPopup(kPopupId);
        pendingOpen_ = false;
    }

    // ImGuiCond_Always — the popup recenters every frame so that web canvas
    // resizes (initial GLFW 800x600 → real CSS size handshake) and any step
    // change that grows the popup don't leave it stuck off-center. Modal
    // popups don't accept user drag, so always-recentering is safe.
    const ImVec2 viewportCenter = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(viewportCenter, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ui::modalSize(560.0f, 0.0f), ImGuiCond_Appearing);

    if (!ImGui::BeginPopupModal(kPopupId, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        return;
    }

    const Step& cur = kSteps[step_];

    ImGui::TextColored(ImVec4(0.40f, 0.75f, 1.0f, 1.0f), "%s", cur.title);
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::TextWrapped("%s", cur.body);
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Step counter + navigation buttons in one row.
    ImGui::TextDisabled("Step %d / %d", step_ + 1, kStepCount);
    ImGui::SameLine();

    const float buttonWidth  = 100.0f;
    const float rowWidth     = ImGui::GetContentRegionAvail().x;
    const float buttonsTotal = (buttonWidth * 3.0f) + (ImGui::GetStyle().ItemSpacing.x * 2.0f);
    ImGui::Dummy(ImVec2(rowWidth - buttonsTotal - 8.0f, 0.0f));
    ImGui::SameLine();

    ImGui::BeginDisabled(step_ == 0);
    if (ImGui::Button("Back", ImVec2(buttonWidth, 0.0f)))
        --step_;
    ImGui::EndDisabled();
    ImGui::SameLine();

    if (step_ + 1 < kStepCount)
    {
        if (ImGui::Button("Next", ImVec2(buttonWidth, 0.0f)))
            ++step_;
        ImGui::SameLine();
        if (ImGui::Button("Skip", ImVec2(buttonWidth, 0.0f)))
        {
            markSeen();
            ImGui::CloseCurrentPopup();
        }
    }
    else
    {
        if (ImGui::Button("Done", ImVec2(buttonWidth, 0.0f)))
        {
            markSeen();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        ImGui::Dummy(ImVec2(buttonWidth, 0.0f));
    }

    ImGui::EndPopup();
}
