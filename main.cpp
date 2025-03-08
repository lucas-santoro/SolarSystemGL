#include "core/Window.h"
#include "core/Shader.h"
#include "objects/Planet.h"
#include <glad/glad.h>

int main()
{
    Window window(800, 600, "SolarSystemGL");

    Shader shader("shaders/VertexShader.glsl", "shaders/FragmentShader.glsl");
    Planet earth(1.0f, 3);
    while (!glfwWindowShouldClose(window.getGLFWwindow()))
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();
        earth.render();

        glfwSwapBuffers(window.getGLFWwindow());
        glfwPollEvents();
    }

    return 0;
}
