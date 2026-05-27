#pragma once

/**
 * @file Platform.h
 * @brief Tiny runtime probes for the host platform — primarily used to pick
 *        mobile-friendly defaults on first launch.
 *
 * Implementation is in `Platform.cpp` and branches on `SOLARSYSTEM_BUILD_WEB`.
 */
namespace platform
{
    /**
     * @brief Best-effort detection of "is this a mobile device?".
     *
     * Web: tries `navigator.userAgentData.mobile` first (a Client Hint that
     * recent Chromium browsers expose), and falls back to a viewport-width
     * heuristic + `'ontouchstart' in window`.
     *
     * Desktop: always returns `false`.
     *
     * Call once during startup to pick defaults — do not poll it per frame.
     */
    bool isMobileDevice();
}
