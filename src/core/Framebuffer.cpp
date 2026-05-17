#include "Framebuffer.h"

#include <utility>

Framebuffer::Framebuffer(int width, int height, bool withDepth)
    : width_(width)
    , height_(height)
    , hasDepth_(withDepth)
{
    create();
}

Framebuffer::~Framebuffer()
{
    destroy();
}

Framebuffer::Framebuffer(Framebuffer&& other) noexcept
    : fbo_(other.fbo_)
    , colorTexture_(other.colorTexture_)
    , depthRenderbuffer_(other.depthRenderbuffer_)
    , width_(other.width_)
    , height_(other.height_)
    , hasDepth_(other.hasDepth_)
{
    other.fbo_               = 0;
    other.colorTexture_      = 0;
    other.depthRenderbuffer_ = 0;
}

Framebuffer& Framebuffer::operator=(Framebuffer&& other) noexcept
{
    if (this != &other)
    {
        destroy();
        fbo_               = other.fbo_;
        colorTexture_      = other.colorTexture_;
        depthRenderbuffer_ = other.depthRenderbuffer_;
        width_             = other.width_;
        height_            = other.height_;
        hasDepth_          = other.hasDepth_;
        other.fbo_               = 0;
        other.colorTexture_      = 0;
        other.depthRenderbuffer_ = 0;
    }
    return *this;
}

void Framebuffer::create()
{
    glGenFramebuffers(1, &fbo_);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);

    glGenTextures(1, &colorTexture_);
    glBindTexture(GL_TEXTURE_2D, colorTexture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width_, height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture_, 0);

    if (hasDepth_)
    {
        glGenRenderbuffers(1, &depthRenderbuffer_);
        glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbuffer_);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width_, height_);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                  GL_RENDERBUFFER, depthRenderbuffer_);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

void Framebuffer::destroy()
{
    if (depthRenderbuffer_) glDeleteRenderbuffers(1, &depthRenderbuffer_);
    if (colorTexture_)      glDeleteTextures(1, &colorTexture_);
    if (fbo_)               glDeleteFramebuffers(1, &fbo_);
    depthRenderbuffer_ = 0;
    colorTexture_      = 0;
    fbo_               = 0;
}

void Framebuffer::bind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
}

void Framebuffer::bindDefault()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::resize(int width, int height)
{
    if (width == width_ && height == height_) return;
    destroy();
    width_  = width;
    height_ = height;
    create();
}
