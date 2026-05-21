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
     * @param width      Initial color-texture width in pixels.
     * @param height     Initial color-texture height in pixels.
     * @param withDepth  When true, also allocate a `GL_DEPTH_COMPONENT24`
     *                   renderbuffer attached at `GL_DEPTH_ATTACHMENT`.
     */
    Framebuffer(int width, int height, bool withDepth);

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

    /// @return The GL handle of the color texture (for sampling in shaders).
    GLuint colorTexture() const { return colorTexture_; }

    /// @return The pixel width of the color texture.
    int width() const  { return width_;  }

    /// @return The pixel height of the color texture.
    int height() const { return height_; }

private:
    void create();
    void destroy();

    GLuint fbo_               = 0;
    GLuint colorTexture_      = 0;
    GLuint depthRenderbuffer_ = 0;
    int    width_             = 0;
    int    height_            = 0;
    bool   hasDepth_          = false;
};
