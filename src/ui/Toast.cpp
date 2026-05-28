#include "Toast.h"

#include <imgui.h>

namespace
{
constexpr float kFadeOutWindow = 0.5f;
constexpr float kToastWidth    = 320.0f;
constexpr float kToastSpacing  = 6.0f;
constexpr float kEdgePadding   = 16.0f;

const glm::vec4 kInfoColor{0.85f, 0.85f, 0.85f, 0.92f};
const glm::vec4 kSuccessColor{0.30f, 0.78f, 0.42f, 0.92f};
const glm::vec4 kWarningColor{0.95f, 0.75f, 0.18f, 0.92f};
const glm::vec4 kErrorColor{0.92f, 0.32f, 0.32f, 0.92f};
} // namespace

void ToastQueue::push(const std::string& message, const glm::vec4& color, float duration)
{
    toasts_.push_back({message, duration, duration, color});
}

void ToastQueue::info(const std::string& message)
{
    push(message, kInfoColor);
}
void ToastQueue::success(const std::string& message)
{
    push(message, kSuccessColor);
}
void ToastQueue::warning(const std::string& message)
{
    push(message, kWarningColor);
}
void ToastQueue::error(const std::string& message)
{
    push(message, kErrorColor);
}

void ToastQueue::update(float deltaTime)
{
    for (auto& toast : toasts_)
    {
        toast.remaining -= deltaTime;
    }
    while (!toasts_.empty() && toasts_.front().remaining <= 0.0f)
    {
        toasts_.pop_front();
    }
}

void ToastQueue::render()
{
    if (toasts_.empty())
        return;

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const ImVec2 workPos          = viewport->WorkPos;
    const ImVec2 workSize         = viewport->WorkSize;
    const float anchorX           = workPos.x + workSize.x - kEdgePadding;
    float cursorY                 = workPos.y + workSize.y - kEdgePadding;

    int index = 0;
    for (auto it = toasts_.rbegin(); it != toasts_.rend(); ++it, ++index)
    {
        const Toast& toast     = *it;
        const float alphaScale = toast.remaining < kFadeOutWindow ? toast.remaining / kFadeOutWindow : 1.0f;

        const ImVec4 background(toast.color.r, toast.color.g, toast.color.b, toast.color.a * alphaScale);

        ImGui::PushStyleColor(ImGuiCol_WindowBg, background);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.05f, 0.05f, 0.05f, 1.0f * alphaScale));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 8.0f));

        const std::string windowId = "##toast" + std::to_string(index);
        ImGui::SetNextWindowSize(ImVec2(kToastWidth, 0.0f));
        ImGui::SetNextWindowPos(ImVec2(anchorX, cursorY), ImGuiCond_Always, ImVec2(1.0f, 1.0f));
        ImGui::Begin(windowId.c_str(), nullptr,
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                         ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings |
                         ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav |
                         ImGuiWindowFlags_NoInputs);

        ImGui::PushTextWrapPos(kToastWidth - 24.0f);
        ImGui::TextUnformatted(toast.message.c_str());
        ImGui::PopTextWrapPos();

        const float renderedHeight = ImGui::GetWindowHeight();
        ImGui::End();

        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(2);

        cursorY -= renderedHeight + kToastSpacing;
    }
}

void ToastQueue::clear()
{
    toasts_.clear();
}
