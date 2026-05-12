#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <glm/gtc/type_ptr.hpp>

/**
 * @file Shader.h
 * @brief GLSL program wrapper: compile, link, and set uniforms with caching.
 */

/**
 * @brief Owns a compiled-and-linked OpenGL program plus a uniform-location cache.
 *
 * The constructor loads vertex + fragment source from disk, compiles, links,
 * and throws `std::runtime_error` if any step fails (with the GL info log in
 * the exception message). Uniform locations are cached on first lookup so
 * `glGetUniformLocation` only runs once per name.
 *
 * Non-copyable and movable (RAII for the GL program handle).
 */
class Shader
{
public:
    /**
     * @brief Compile and link a shader program from two source files.
     * @param vertexPath   Path to the vertex shader GLSL file.
     * @param fragmentPath Path to the fragment shader GLSL file.
     * @throws std::runtime_error if either file is missing or compilation/linking fails.
     */
    Shader(const std::string& vertexPath, const std::string& fragmentPath);

    /** @brief Delete the underlying GL program. */
    ~Shader();

    Shader(const Shader&)            = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    /** @brief Bind this program as the current GL program. */
    void use();

    /** @return The raw GL program handle. */
    unsigned int getID() const;

    /** @brief Upload a 4x4 matrix uniform by name. */
    void setMat4(const std::string& name, const glm::mat4& mat);

    /** @brief Upload a single vec3 uniform by name. */
    void setVec3(const std::string& name, const glm::vec3& vec);

    /** @brief Upload an array of vec3 uniforms. The shader-side array must match @p values.size(). */
    void setVec3Array(const std::string& name, const std::vector<glm::vec3>& values);

    /** @brief Upload a single float uniform by name. */
    void setFloat(const std::string& name, float value);

    /** @brief Upload an array of float uniforms. */
    void setFloatArray(const std::string& name, const std::vector<float>& values);

    /** @brief Upload a single int uniform by name. */
    void setInt(const std::string& name, int value);

private:
    unsigned int                           ID = 0;
    std::unordered_map<std::string, int>   uniformLocations;

    int  uniformLocation(const std::string& name);
    void compileAndLink(const std::string& vertexCode,
                        const std::string& fragmentCode,
                        const std::string& vertexPath,
                        const std::string& fragmentPath);
    static std::string readFile(const std::string& filePath);
};
