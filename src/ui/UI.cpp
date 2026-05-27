#include "ui/UI.h"

#include <algorithm>

namespace ui
{

bool isMobileViewport()
{
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    return vp != nullptr && vp->Size.x < kMobileBreakpointPx;
}

bool isCompactViewport()
{
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    return vp != nullptr && vp->Size.x < kCompactBreakpointPx;
}

ImVec2 modalSize(float desiredWidth, float desiredHeight)
{
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    if (vp == nullptr) return { desiredWidth, desiredHeight };

    const float maxW = vp->Size.x * 0.9f;
    const float maxH = vp->Size.y * 0.9f;

    // desired==0 means "auto-fit" — preserve that signal by passing it through.
    const float outW = (desiredWidth  > 0.0f) ? std::min(desiredWidth,  maxW) : 0.0f;
    const float outH = (desiredHeight > 0.0f) ? std::min(desiredHeight, maxH) : 0.0f;
    return { outW, outH };
}

void applyResponsiveStyle()
{
    // Cache the last applied mode so we only do the (relatively expensive)
    // style mutation when the breakpoint flips. The first call always applies.
    static int  lastMobile = -1;
    const  bool mobile     = isMobileViewport();
    const  int  currentMobile = mobile ? 1 : 0;
    if (currentMobile == lastMobile) return;
    lastMobile = currentMobile;

    ImGuiIO&    io    = ImGui::GetIO();
    ImGuiStyle& style = ImGui::GetStyle();

    if (mobile)
    {
        io.FontGlobalScale = 1.35f;
        // ScaleAllSizes multiplies the current style by the factor — we want
        // to scale relative to the desktop baseline, so reset first then
        // multiply by the mobile factor.
        ImGui::StyleColorsDark();          // resets non-color settings too? no — colors only.
        // Instead of trying to "reset" sizes, we accept that this runs once
        // per breakpoint flip and apply the desired multiplier from a fresh
        // baseline by recomputing key sizes explicitly.
        style.WindowPadding    = ImVec2(12.0f, 12.0f);
        style.FramePadding     = ImVec2(8.0f,  6.0f);
        style.ItemSpacing      = ImVec2(10.0f, 8.0f);
        style.ItemInnerSpacing = ImVec2(8.0f,  6.0f);
        style.GrabMinSize      = 18.0f;
        style.ScrollbarSize    = 18.0f;
    }
    else
    {
        io.FontGlobalScale = 1.0f;
        style.WindowPadding    = ImVec2(8.0f,  8.0f);
        style.FramePadding     = ImVec2(4.0f,  3.0f);
        style.ItemSpacing      = ImVec2(8.0f,  4.0f);
        style.ItemInnerSpacing = ImVec2(4.0f,  4.0f);
        style.GrabMinSize      = 10.0f;
        style.ScrollbarSize    = 14.0f;
    }
}

} // namespace ui
