#pragma once
#include <string>

class Shader
{
public:
    Shader(const std::string &vertexPath, const std::string &fragmentPath);
    ~Shader();
    void use();
    unsigned int getID() const;

private:
    unsigned int ID;
    bool compileShader(const std::string &vertexCode, const std::string &fragmentCode);
    std::string readFile(const std::string &filePath);
};
