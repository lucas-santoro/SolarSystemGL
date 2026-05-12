#include "core/Shader.h"
#include <glad/glad.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <utility>

Shader::Shader(const std::string &vertexPath, const std::string &fragmentPath)
{
    const std::string vertexCode   = readFile(vertexPath);
    const std::string fragmentCode = readFile(fragmentPath);
    compileAndLink(vertexCode, fragmentCode, vertexPath, fragmentPath);
}

Shader::~Shader()
{
    if (ID) glDeleteProgram(ID);
}

Shader::Shader(Shader&& other) noexcept
    : ID(other.ID), uniformLocations(std::move(other.uniformLocations))
{
    other.ID = 0;
}

Shader& Shader::operator=(Shader&& other) noexcept
{
    if (this != &other)
    {
        if (ID) glDeleteProgram(ID);
        ID = other.ID;
        uniformLocations = std::move(other.uniformLocations);
        other.ID = 0;
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

void Shader::compileAndLink(const std::string &vertexCode,
                            const std::string &fragmentCode,
                            const std::string &vertexPath,
                            const std::string &fragmentPath)
{
    auto compileStage = [](GLenum type, const char* src, const std::string& label) -> GLuint
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
            throw std::runtime_error("Failed to compile " + label + ":\n" + log);
        }
        return shader;
    };

    GLuint vertex   = compileStage(GL_VERTEX_SHADER,   vertexCode.c_str(),   vertexPath);
    GLuint fragment = compileStage(GL_FRAGMENT_SHADER, fragmentCode.c_str(), fragmentPath);

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
        throw std::runtime_error("Failed to link shader program ("
                                 + vertexPath + ", " + fragmentPath + "):\n" + log);
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

std::string Shader::readFile(const std::string &filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open shader file: " + filePath);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int Shader::uniformLocation(const std::string &name)
{
    auto it = uniformLocations.find(name);
    if (it != uniformLocations.end()) return it->second;

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

void Shader::setMat4(const std::string &name, const glm::mat4 &mat)
{
    glUniformMatrix4fv(uniformLocation(name), 1, GL_FALSE, glm::value_ptr(mat));
}

void Shader::setVec3(const std::string &name, const glm::vec3 &value)
{
    glUniform3fv(uniformLocation(name), 1, &value[0]);
}

void Shader::setVec3Array(const std::string &name, const std::vector<glm::vec3> &values)
{
    glUniform3fv(uniformLocation(name), static_cast<GLsizei>(values.size()), glm::value_ptr(values[0]));
}

void Shader::setFloat(const std::string &name, float value)
{
    glUniform1f(uniformLocation(name), value);
}

void Shader::setFloatArray(const std::string &name, const std::vector<float> &values)
{
    glUniform1fv(uniformLocation(name), static_cast<GLsizei>(values.size()), values.data());
}

void Shader::setInt(const std::string &name, int value)
{
    glUniform1i(uniformLocation(name), value);
}
