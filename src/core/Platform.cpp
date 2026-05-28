#include "core/Platform.h"

#ifdef SOLARSYSTEM_BUILD_WEB
#include <emscripten.h>
#endif

namespace platform
{

#ifdef SOLARSYSTEM_BUILD_WEB

namespace
{
// clang-format off
// The EM_JS body is JavaScript, not C++; clang-format would mangle it
// (splitting `===`, `=>`), so the directive above tells it to skip this region.
    // navigator.userAgentData.mobile is undefined in Safari/Firefox, so we
    // fall back to a viewport-width + touch-capability heuristic. Threshold
    // chosen to match the CSS breakpoint used by the responsive UI.
    EM_JS(int, jsIsMobileDevice, (), {
        try {
            if (navigator.userAgentData && typeof navigator.userAgentData.mobile === 'boolean') {
                return navigator.userAgentData.mobile ? 1 : 0;
            }
        } catch (e) { /* shrug */ }

        const narrow      = (window.innerWidth || 0) < 600;
        const hasTouch    = ('ontouchstart' in window) ||
                            (navigator.maxTouchPoints && navigator.maxTouchPoints > 0);
        return (narrow && hasTouch) ? 1 : 0;
    });
// clang-format on
} // namespace

bool isMobileDevice()
{
    return jsIsMobileDevice() != 0;
}

#else // !SOLARSYSTEM_BUILD_WEB

bool isMobileDevice()
{
    return false;
}

#endif

} // namespace platform
