#pragma once
#include <string>
#include <glm/gtc/type_ptr.hpp>
#include <vector>


class Shader
{
public:
    Shader(const std::string &vertexPath, const std::string &fragmentPath);
    ~Shader();
    void use();
    unsigned int getID() const;
    void setMat4(const std::string &name, const glm::mat4 &mat);
	void setVec3(const std::string &name, const glm::vec3 &vec);
    void setVec3Array(const std::string &name, const std::vector<glm::vec3> &values);
    void setFloatArray(const std::string &name, const std::vector<float> &values);
    void setInt(const std::string &name, int value);

private:
    unsigned int ID;
    bool compileShader(const std::string &vertexCode, const std::string &fragmentCode);
    std::string readFile(const std::string &filePath);
};
