#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <glm/gtc/type_ptr.hpp>

class Shader
{
public:
    Shader(const std::string &vertexPath, const std::string &fragmentPath);
    ~Shader();

    Shader(const Shader&)            = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    void use();
    unsigned int getID() const;

    void setMat4(const std::string &name, const glm::mat4 &mat);
    void setVec3(const std::string &name, const glm::vec3 &vec);
    void setVec3Array(const std::string &name, const std::vector<glm::vec3> &values);
    void setFloat(const std::string &name, float value);
    void setFloatArray(const std::string &name, const std::vector<float> &values);
    void setInt(const std::string &name, int value);

private:
    unsigned int ID = 0;
    std::unordered_map<std::string, int> uniformLocations;

    int  uniformLocation(const std::string &name);
    void compileAndLink(const std::string &vertexCode,
                        const std::string &fragmentCode,
                        const std::string &vertexPath,
                        const std::string &fragmentPath);
    static std::string readFile(const std::string &filePath);
};
