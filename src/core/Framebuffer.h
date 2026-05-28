#pragma once

#include "core/GL.h"

/**
 * @file Framebuffer.h
 * @brief Thin RAII wrapper around a single-color-attachment FBO.
 */

/**
 * @brief Owns an FBO + a color texture + (optionally) a depth renderbuffer.
 *
 * Used for offscreen render targets — the scene FBO and the two bloom
 * ping-pong FBOs all share this type. Non-copyable and movable so it can
 * live inside an `Application` without leaking GL handles.
 */
class Framebuffer
{
public:
    /**
     * @brief Allocate the FBO with the requested dimensions.
     * @param width      Initial color-attachment width in pixels.
     * @param height     Initial color-attachment height in pixels.
     * @param withDepth  When true, also allocate a `GL_DEPTH_COMPONENT24`
     *                   renderbuffer attached at `GL_DEPTH_ATTACHMENT`.
     * @param samples    `1` for a single-sample color texture (default).
     *                   `>1` switches the color (and depth, if present) to a
     *                   multisample renderbuffer — the FBO can be rendered to
     *                   but `colorTexture()` returns 0; resolve via
     *                   `blitColorTo` into a single-sample peer first.
     */
    Framebuffer(int width, int height, bool withDepth, int samples = 1);

    /// Release the FBO and any GL handles it owns.
    ~Framebuffer();

    Framebuffer(const Framebuffer&)            = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;
    Framebuffer(Framebuffer&& other) noexcept;
    Framebuffer& operator=(Framebuffer&& other) noexcept;

    /// Bind for both read and write.
    void bind() const;

    /// Bind the default framebuffer (the window's back buffer).
    static void bindDefault();

    /**
     * @brief Tear down and recreate at a new size — typically called from
     *        the GLFW framebuffer-size callback.
     */
    void resize(int width, int height);

    /**
     * @brief Tear down and recreate at a new sample count, preserving the
     *        current dimensions. Used when the user toggles MSAA at runtime.
     *
     * No-op if @p samples matches the current count.
     */
    void setSamples(int samples);

    /**
     * @brief Resolve this FBO's color into @p dst via `glBlitFramebuffer`.
     *
     * Intended use: render multisample into this FBO, then resolve to a
     * single-sample peer that can be sampled as a texture by a subsequent
     * fullscreen pass.
     */
    void blitColorTo(const Framebuffer& dst) const;

    /// @return GL handle of the color texture, or 0 if this FBO is multisample.
    GLuint colorTexture() const
    {
        return colorTexture_;
    }

    /// @return The pixel width of the color attachment.
    int width() const
    {
        return width_;
    }

    /// @return The pixel height of the color attachment.
    int height() const
    {
        return height_;
    }

    /// @return Sample count (1 = single-sample, >1 = MSAA renderbuffer).
    int samples() const
    {
        return samples_;
    }

private:
    void create();
    void destroy();

    GLuint fbo_               = 0;
    GLuint colorTexture_      = 0; ///< Used when samples_ == 1.
    GLuint colorRenderbuffer_ = 0; ///< Used when samples_ > 1.
    GLuint depthRenderbuffer_ = 0;
    int width_                = 0;
    int height_               = 0;
    int samples_              = 1;
    bool hasDepth_            = false;
};
