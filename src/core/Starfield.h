#pragma once

#include "core/GL.h"
#include "core/Shader.h"
#include <glm/glm.hpp>
#include <string>

/**
 * @file Starfield.h
 * @brief Procedural multi-layer night-sky renderer (the background star pass).
 */

/**
 * @brief Quality tier for the procedural starfield.
 *
 * Selects how many layers the sky shader evaluates. Higher tiers add density
 * octaves and effects at GPU cost, so this doubles as a perf lever on mobile
 * (defaulted lower for touch devices, see `Application`). The value is uploaded
 * straight to the shader as the `uQuality` uniform, so the enumerators are
 * ordered and contiguous on purpose.
 */
enum class StarfieldQuality
{
    Off    = 0, ///< No stars — the sky is left black.
    Low    = 1, ///< Bright foreground stars only.
    Medium = 2, ///< + a denser mid-field layer and full color variation.
    High   = 3, ///< + a faint dust layer, the Milky Way band, and twinkle.
};

/**
 * @brief Draws a procedural starfield as a single fullscreen pass.
 *
 * Self-contained: owns its compiled sky program and its own fullscreen-quad
 * VAO/VBO, so the only thing a caller needs is one #render call per frame. The
 * stars are computed entirely in the fragment shader from the view direction
 * (so they sit "at infinity" — translating the camera doesn't shift them);
 * there is no geometry and no texture, keeping the renderer asset-free.
 *
 * Non-copyable and movable (RAII for the GL buffer handles), mirroring `Shader`.
 */
class Starfield
{
public:
    /**
     * @brief Compile the sky program and build the fullscreen quad.
     * @param vertexSource   GLSL source for the fullscreen-quad vertex stage.
     * @param fragmentSource GLSL source for the layered starfield fragment stage.
     * @throws std::runtime_error if the shader fails to compile or link.
     */
    Starfield(const std::string& vertexSource, const std::string& fragmentSource);

    /** @brief Delete the owned VAO/VBO. */
    ~Starfield();

    Starfield(const Starfield&)            = delete;
    Starfield& operator=(const Starfield&) = delete;
    Starfield(Starfield&& other) noexcept;
    Starfield& operator=(Starfield&& other) noexcept;

    /**
     * @brief Render the sky for one frame into the currently bound framebuffer.
     *
     * Disables depth testing for the pass and restores it afterwards. When
     * @p quality is `StarfieldQuality::Off` this returns immediately without
     * drawing — the caller is expected to have already cleared to black.
     *
     * @param view        Active view matrix (inverted on the GPU to get the ray).
     * @param projection  Active projection matrix (likewise inverted).
     * @param timeSeconds  Monotonic time in seconds, drives the High-tier twinkle.
     * @param quality     Layer tier to evaluate (see #StarfieldQuality).
     */
    void render(const glm::mat4& view, const glm::mat4& projection, float timeSeconds,
                StarfieldQuality quality);

private:
    Shader shader_;
    GLuint vao_ = 0;
    GLuint vbo_ = 0;

    void destroy() noexcept;
};
