#include "core/Window.h"

Window::Window(int width, int height, const std::string &title)
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
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Err - GLAD" << std::endl;
        return false;
    }

    return true;
}

void Window::processInput()
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }
}

void Window::run()
{
    while (!glfwWindowShouldClose(window))
    {
        processInput();
        glClear(GL_COLOR_BUFFER_BIT);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

GLFWwindow* Window::getGLFWwindow()
{
    return window;
}
