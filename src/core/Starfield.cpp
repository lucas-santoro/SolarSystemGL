#include "core/Starfield.h"

#include <utility>

namespace
{
// Fullscreen unit quad as a triangle strip (matches the sky vertex shader's
// `layout(location = 0) in vec2 aPos`). Kept local to this renderer so the
// Starfield is fully self-contained.
constexpr float kFullscreenQuad[] = {
    -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f,
};
} // namespace

Starfield::Starfield(const std::string& vertexSource, const std::string& fragmentSource)
    : shader_(vertexSource, fragmentSource)
{
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kFullscreenQuad), kFullscreenQuad, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

Starfield::~Starfield()
{
    destroy();
}

Starfield::Starfield(Starfield&& other) noexcept
    : shader_(std::move(other.shader_)), vao_(other.vao_), vbo_(other.vbo_)
{
    other.vao_ = 0;
    other.vbo_ = 0;
}

Starfield& Starfield::operator=(Starfield&& other) noexcept
{
    if (this != &other)
    {
        destroy();
        shader_    = std::move(other.shader_);
        vao_       = other.vao_;
        vbo_       = other.vbo_;
        other.vao_ = 0;
        other.vbo_ = 0;
    }
    return *this;
}

void Starfield::destroy() noexcept
{
    if (vbo_ != 0)
        glDeleteBuffers(1, &vbo_);
    if (vao_ != 0)
        glDeleteVertexArrays(1, &vao_);
    vao_ = 0;
    vbo_ = 0;
}

void Starfield::render(const glm::mat4& view, const glm::mat4& projection, float timeSeconds,
                       StarfieldQuality quality)
{
    if (quality == StarfieldQuality::Off)
        return;

    shader_.use();
    shader_.setMat4("invView", glm::inverse(view));
    shader_.setMat4("invProj", glm::inverse(projection));
    shader_.setFloat("uTime", timeSeconds);
    shader_.setInt("uQuality", static_cast<int>(quality));

    glDisable(GL_DEPTH_TEST);
    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
}
