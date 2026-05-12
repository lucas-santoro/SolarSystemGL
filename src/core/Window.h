#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>

class Window
{
public:
    Window(int width, int height, const std::string &title);
    ~Window();

    GLFWwindow* getGLFWwindow();

private:
    int width, height;
    std::string title;
    GLFWwindow *window;

    bool init();
};
