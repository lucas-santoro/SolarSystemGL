#include "core/Shader.h"
#include "core/GL.h"
#include <iostream>
#include <stdexcept>
#include <utility>

namespace
{
#ifdef SOLARSYSTEM_BUILD_WEB
/// Rewrite a desktop GLSL 3.30 core source into something WebGL2 accepts.
/// WebGL2 only takes GLSL ES 3.00 shaders, so we swap the version pragma
/// and inject default precision qualifiers — required for fragment shaders,
/// harmless for vertex shaders. The body of the shader is otherwise
/// `in/out`-flavoured already and stays compatible.
std::string toWebGL2GLSL(const std::string& src)
{
    const std::string desktopVersion = "#version 330 core";
    const std::string webglPreamble  = "#version 300 es\n"
                                       "precision highp float;\n"
                                       "precision highp int;\n"
                                       "precision mediump sampler2D;\n";

    std::string result = src;
    const auto pos     = result.find(desktopVersion);
    if (pos != std::string::npos)
    {
        result.replace(pos, desktopVersion.length(), webglPreamble);
    }
    return result;
}
#endif
} // namespace

Shader::Shader(const std::string& vertexSource, const std::string& fragmentSource)
{
#ifdef SOLARSYSTEM_BUILD_WEB
    compileAndLink(toWebGL2GLSL(vertexSource), toWebGL2GLSL(fragmentSource));
#else
    compileAndLink(vertexSource, fragmentSource);
#endif
}

Shader::~Shader()
{
    if (ID)
        glDeleteProgram(ID);
}

Shader::Shader(Shader&& other) noexcept : ID(other.ID), uniformLocations(std::move(other.uniformLocations))
{
    other.ID = 0;
}

Shader& Shader::operator=(Shader&& other) noexcept
{
    if (this != &other)
    {
        if (ID)
            glDeleteProgram(ID);
        ID               = other.ID;
        uniformLocations = std::move(other.uniformLocations);
        other.ID         = 0;
    }
    return *this;
}

void Shader::use()
{
    glUseProgram(ID);
}

unsigned int Shader::getID() const
{
    return ID;
}

void Shader::compileAndLink(const std::string& vertexCode, const std::string& fragmentCode)
{
    auto compileStage = [](GLenum type, const char* src, const char* label) -> GLuint
    {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);
        GLint success = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char log[1024];
            glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
            glDeleteShader(shader);
            throw std::runtime_error(std::string("Failed to compile ") + label + ":\n" + log);
        }
        return shader;
    };

    GLuint vertex   = compileStage(GL_VERTEX_SHADER, vertexCode.c_str(), "vertex shader");
    GLuint fragment = compileStage(GL_FRAGMENT_SHADER, fragmentCode.c_str(), "fragment shader");

    ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    glLinkProgram(ID);

    GLint success = 0;
    glGetProgramiv(ID, GL_LINK_STATUS, &success);
    if (!success)
    {
        char log[1024];
        glGetProgramInfoLog(ID, sizeof(log), nullptr, log);
        glDeleteProgram(ID);
        ID = 0;
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        throw std::runtime_error(std::string("Failed to link shader program:\n") + log);
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

int Shader::uniformLocation(const std::string& name)
{
    auto it = uniformLocations.find(name);
    if (it != uniformLocations.end())
        return it->second;

    const int location = glGetUniformLocation(ID, name.c_str());
    uniformLocations.emplace(name, location);
    if (location < 0)
    {
        // Cache the miss so we don't keep calling glGetUniformLocation; the
        // glUniform* call below will silently no-op which is the standard
        // behavior, but at least surface it once.
        std::cerr << "Shader: uniform '" << name << "' not found (or optimized out)\n";
    }
    return location;
}

void Shader::setMat4(const std::string& name, const glm::mat4& mat)
{
    glUniformMatrix4fv(uniformLocation(name), 1, GL_FALSE, glm::value_ptr(mat));
}

void Shader::setVec3(const std::string& name, const glm::vec3& value)
{
    glUniform3fv(uniformLocation(name), 1, &value[0]);
}

void Shader::setVec3Array(const std::string& name, const std::vector<glm::vec3>& values)
{
    glUniform3fv(uniformLocation(name), static_cast<GLsizei>(values.size()), glm::value_ptr(values[0]));
}

void Shader::setFloat(const std::string& name, float value)
{
    glUniform1f(uniformLocation(name), value);
}

void Shader::setFloatArray(const std::string& name, const std::vector<float>& values)
{
    glUniform1fv(uniformLocation(name), static_cast<GLsizei>(values.size()), values.data());
}

void Shader::setInt(const std::string& name, int value)
{
    glUniform1i(uniformLocation(name), value);
}
