#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>

/**
 * @file Window.h
 * @brief Thin RAII wrapper around a GLFW window and its OpenGL context.
 */

/**
 * @brief Owns a GLFW window and the OpenGL context attached to it.
 *
 * The constructor initializes GLFW (if not already), creates the window, makes
 * its OpenGL context current, and loads OpenGL function pointers via GLAD.
 * The destructor destroys the window and terminates GLFW.
 */
class Window
{
public:
    /**
     * @brief Create a window and its OpenGL context.
     * @param width  Initial framebuffer width in pixels.
     * @param height Initial framebuffer height in pixels.
     * @param title  Window title shown by the OS / window manager.
     */
    Window(int width, int height, const std::string& title);

    /**
     * @brief Destroy the window and terminate GLFW.
     */
    ~Window();

    /**
     * @brief Access the underlying GLFW handle.
     * @return Raw GLFW window pointer (non-owning).
     */
    GLFWwindow* getGLFWwindow();

private:
    int          width;
    int          height;
    std::string  title;
    GLFWwindow*  window;

    bool init();
};
