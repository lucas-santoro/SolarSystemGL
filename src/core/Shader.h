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
 * The constructor accepts vertex + fragment source code directly — there is
 * no runtime file I/O. Shader sources are embedded into the executable at
 * configure time (see `EmbeddedShaders.h`). Compilation or linker failures
 * throw `std::runtime_error` with the GL info log in the message. Uniform
 * locations are cached on first lookup so `glGetUniformLocation` only runs
 * once per name.
 *
 * Non-copyable and movable (RAII for the GL program handle).
 */
class Shader
{
public:
    /**
     * @brief Compile and link a shader program from raw GLSL source.
     * @param vertexSource   GLSL source for the vertex stage.
     * @param fragmentSource GLSL source for the fragment stage.
     * @throws std::runtime_error if compilation or linking fails.
     */
    Shader(const std::string& vertexSource, const std::string& fragmentSource);

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
    unsigned int ID = 0;
    std::unordered_map<std::string, int> uniformLocations;

    int uniformLocation(const std::string& name);
    void compileAndLink(const std::string& vertexCode, const std::string& fragmentCode);
};
