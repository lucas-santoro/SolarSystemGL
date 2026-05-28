#pragma once

#include <imgui.h>

/**
 * @file UI.h
 * @brief Free utilities shared by every ImGui panel/modal in the project —
 *        viewport breakpoint detection, responsive sizing, and style scaling.
 *
 * These live outside `UIManager` so any UI source can include them without
 * dragging in the larger state holder.
 */
namespace ui
{
/// Viewport widths below this threshold are treated as "mobile" — drives
/// font scaling and tap-target enlargement (phones).
constexpr float kMobileBreakpointPx = 600.0f;

/// Viewport widths below this threshold are treated as "compact" — drives
/// Actionbar reflow into an overflow menu. Wider than `kMobileBreakpointPx`
/// because the full desktop actionbar needs ~900px to fit cleanly without
/// truncating buttons or overlapping the right-anchored FPS readout.
constexpr float kCompactBreakpointPx = 960.0f;

/// True if the current ImGui main viewport is narrower than the mobile
/// breakpoint. Cheap (single `GetMainViewport()` call); safe to call
/// per-widget if needed.
bool isMobileViewport();

/// True if the current ImGui main viewport is narrower than the compact
/// breakpoint. Use this for layout reflow that should kick in earlier than
/// the font/tap-target changes — notably the Actionbar overflow menu.
bool isCompactViewport();

/**
 * @brief Compute a responsive size for a modal: clamp the requested
 *        dimensions to 90% of the viewport so popups always fit.
 *
 * @param desiredWidth  Hard-coded "desktop" width. Pass 0 for auto-fit.
 * @param desiredHeight Hard-coded "desktop" height. Pass 0 for auto-fit
 *                      (ImGui's AlwaysAutoResize flag still applies).
 * @return ImVec2 ready for `ImGui::SetNextWindowSize`.
 */
ImVec2 modalSize(float desiredWidth, float desiredHeight);

/**
 * @brief Apply font + padding scaling appropriate for the current
 *        viewport width. Mobile gets 1.35× font + 1.3× padding/rounding;
 *        desktop reverts to the base style.
 *
 * Idempotent — internally caches the last applied breakpoint and short-
 * circuits when called repeatedly with no change. Safe to call every
 * frame.
 */
void applyResponsiveStyle();
} // namespace ui
