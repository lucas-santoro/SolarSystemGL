#include "core/Window.h"

Window::Window(int width, int height, const std::string& title)
    : width(width), height(height), title(title), window(nullptr)
{
    if (!init())
    {
        std::cerr << "Err - Window" << std::endl;
    }
}

Window::~Window()
{
    glfwDestroyWindow(window);
    glfwTerminate();
}

bool Window::init()
{
    if (!glfwInit())
    {
        std::cerr << "Err - GLFW" << std::endl;
        return false;
    }

    window = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);
    if (!window)
    {
        std::cerr << "Err - GLFW - Window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);
#ifndef SOLARSYSTEM_BUILD_WEB
    // Emscripten resolves WebGL calls via its JS glue layer — no loader
    // needed. Desktop builds still pull function pointers through glad.
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
    {
        std::cerr << "Err - GLAD" << std::endl;
        return false;
    }
#endif

    return true;
}

GLFWwindow* Window::getGLFWwindow()
{
    return window;
}
