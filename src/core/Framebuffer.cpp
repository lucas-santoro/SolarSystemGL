#include "Framebuffer.h"

#include <utility>

Framebuffer::Framebuffer(int width, int height, bool withDepth, int samples)
    : width_(width), height_(height), samples_(samples < 1 ? 1 : samples), hasDepth_(withDepth)
{
    create();
}

Framebuffer::~Framebuffer()
{
    destroy();
}

Framebuffer::Framebuffer(Framebuffer&& other) noexcept
    : fbo_(other.fbo_), colorTexture_(other.colorTexture_), colorRenderbuffer_(other.colorRenderbuffer_),
      depthRenderbuffer_(other.depthRenderbuffer_), width_(other.width_), height_(other.height_),
      samples_(other.samples_), hasDepth_(other.hasDepth_)
{
    other.fbo_               = 0;
    other.colorTexture_      = 0;
    other.colorRenderbuffer_ = 0;
    other.depthRenderbuffer_ = 0;
}

Framebuffer& Framebuffer::operator=(Framebuffer&& other) noexcept
{
    if (this != &other)
    {
        destroy();
        fbo_                     = other.fbo_;
        colorTexture_            = other.colorTexture_;
        colorRenderbuffer_       = other.colorRenderbuffer_;
        depthRenderbuffer_       = other.depthRenderbuffer_;
        width_                   = other.width_;
        height_                  = other.height_;
        samples_                 = other.samples_;
        hasDepth_                = other.hasDepth_;
        other.fbo_               = 0;
        other.colorTexture_      = 0;
        other.colorRenderbuffer_ = 0;
        other.depthRenderbuffer_ = 0;
    }
    return *this;
}

void Framebuffer::create()
{
    glGenFramebuffers(1, &fbo_);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);

    if (samples_ > 1)
    {
        // MSAA path: color and (optional) depth become multisample
        // renderbuffers. The FBO is not directly samplable as a texture —
        // callers resolve via blitColorTo() into a single-sample peer.
        glGenRenderbuffers(1, &colorRenderbuffer_);
        glBindRenderbuffer(GL_RENDERBUFFER, colorRenderbuffer_);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples_, GL_RGBA8, width_, height_);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colorRenderbuffer_);
    }
    else
    {
        glGenTextures(1, &colorTexture_);
        glBindTexture(GL_TEXTURE_2D, colorTexture_);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width_, height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture_, 0);
    }

    if (hasDepth_)
    {
        glGenRenderbuffers(1, &depthRenderbuffer_);
        glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbuffer_);
        if (samples_ > 1)
        {
            glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples_, GL_DEPTH_COMPONENT24, width_,
                                             height_);
        }
        else
        {
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width_, height_);
        }
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderbuffer_);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

void Framebuffer::destroy()
{
    if (depthRenderbuffer_)
        glDeleteRenderbuffers(1, &depthRenderbuffer_);
    if (colorRenderbuffer_)
        glDeleteRenderbuffers(1, &colorRenderbuffer_);
    if (colorTexture_)
        glDeleteTextures(1, &colorTexture_);
    if (fbo_)
        glDeleteFramebuffers(1, &fbo_);
    depthRenderbuffer_ = 0;
    colorRenderbuffer_ = 0;
    colorTexture_      = 0;
    fbo_               = 0;
}

void Framebuffer::bind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
}

void Framebuffer::blitColorTo(const Framebuffer& dst) const
{
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo_);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst.fbo_);
    glBlitFramebuffer(0, 0, width_, height_, 0, 0, dst.width_, dst.height_, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void Framebuffer::bindDefault()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::resize(int width, int height)
{
    if (width == width_ && height == height_)
        return;
    destroy();
    width_  = width;
    height_ = height;
    create();
}

void Framebuffer::setSamples(int samples)
{
    const int clamped = samples < 1 ? 1 : samples;
    if (clamped == samples_)
        return;
    destroy();
    samples_ = clamped;
    create();
}
