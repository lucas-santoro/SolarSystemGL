#include "StartMenu.h"

#include <imgui.h>

namespace
{
    constexpr float kPanelWidth   = 360.0f;
    constexpr float kButtonWidth  = 260.0f;
    constexpr float kButtonHeight = 44.0f;

    const ImVec4 kPanelBackground{ 0.05f, 0.06f, 0.10f, 0.92f };
}

StartMenuAction StartMenu::render(int viewportWidth, int viewportHeight)
{
    StartMenuAction action = StartMenuAction::None;

    ImGui::SetNextWindowPos(ImVec2(static_cast<float>(viewportWidth)  * 0.5f,
                                   static_cast<float>(viewportHeight) * 0.5f),
                            ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(kPanelWidth, 0.0f));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,  ImVec2(28.0f, 28.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,  6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,    ImVec2(8.0f, 10.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, kPanelBackground);

    ImGui::Begin("##startmenu", nullptr,
                 ImGuiWindowFlags_NoTitleBar  | ImGuiWindowFlags_NoResize  |
                 ImGuiWindowFlags_NoMove      | ImGuiWindowFlags_NoCollapse |
                 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);

    const float panelWidth = ImGui::GetWindowSize().x;

    auto centeredText = [panelWidth](const char* text)
    {
        const float textWidth = ImGui::CalcTextSize(text).x;
        ImGui::SetCursorPosX((panelWidth - textWidth) * 0.5f);
        ImGui::TextUnformatted(text);
    };

    auto centeredDisabledText = [panelWidth](const char* text)
    {
        const float textWidth = ImGui::CalcTextSize(text).x;
        ImGui::SetCursorPosX((panelWidth - textWidth) * 0.5f);
        ImGui::TextDisabled("%s", text);
    };

    centeredText("SolarSystemGL");
    centeredDisabledText("An OpenGL N-body sandbox");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    const float buttonOffsetX = (panelWidth - kButtonWidth) * 0.5f;
    const ImVec2 buttonSize(kButtonWidth, kButtonHeight);

    ImGui::SetCursorPosX(buttonOffsetX);
    if (ImGui::Button("Start Simulation", buttonSize)) action = StartMenuAction::Start;

    ImGui::SetCursorPosX(buttonOffsetX);
    ImGui::BeginDisabled(true);
    ImGui::Button("Load Save", buttonSize);
    ImGui::EndDisabled();

    ImGui::SetCursorPosX(buttonOffsetX);
    ImGui::BeginDisabled(true);
    ImGui::Button("Settings", buttonSize);
    ImGui::EndDisabled();

    ImGui::Spacing();

    ImGui::SetCursorPosX(buttonOffsetX);
    if (ImGui::Button("Quit", buttonSize)) action = StartMenuAction::Quit;

    ImGui::End();

    ImGui::PopStyleColor(1);
    ImGui::PopStyleVar(4);

    return action;
}
