#include <glad/glad.h>
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"

#include "core/Window.h"
#include "core/Shader.h"
#include "objects/Planet.h"
#include "core/Camera.h"
#include "core/Grid.h"

void processInput(Window& window, Camera& camera, float deltaTime);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);

Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
bool showPlanetWindow = false;
float lastX = 400, lastY = 300;
bool firstMouse = true;
float deltaTime = 0.0f;
float lastFrame = 0.0f;

int main()
{
    Window window(800, 600, "SolarSystemGL");
    glfwSetCursorPosCallback(window.getGLFWwindow(), mouse_callback);
    glfwSetInputMode(window.getGLFWwindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    Shader shader("shaders/VertexShader.glsl", "shaders/FragmentShader.glsl");
    Planet earth(1.0f, 3);
    Grid grid(10.0f, 10, -1.0f);

    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window.getGLFWwindow(), true);
    ImGui_ImplOpenGL3_Init("#version 330");

    while (!glfwWindowShouldClose(window.getGLFWwindow()))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window, camera, deltaTime);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        shader.use();

        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
        glm::mat4 model = glm::mat4(1.0f);

        shader.setMat4("view", view);
        shader.setMat4("projection", projection);
        shader.setMat4("model", model);

        earth.render();
        grid.draw();

        double mouseX, mouseY;
        glfwGetCursorPos(window.getGLFWwindow(), &mouseX, &mouseY);

        glm::vec3 rayDirection = camera.getRayFromMouse(mouseX, mouseY, 800, 600, view, projection);
        glm::vec3 rayOrigin = camera.getPosition();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (earth.intersectsRay(camera.getPosition(), rayDirection))
        {
            glm::vec3 planetScreenPos = earth.getPosition() + glm::vec3(0.0f, earth.getRadius() + 0.2f, 0.0f);
            glm::vec2 screenPos = camera.worldToScreen(planetScreenPos, view, projection, 800, 600);

            if (screenPos.x >= 0 && screenPos.y >= 0 && screenPos.x <= 800 && screenPos.y <= 600)
            {
                ImGui::SetNextWindowPos(ImVec2(screenPos.x, screenPos.y));
                ImGui::Begin("##PlanetName", nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
                ImGui::Text("Earth");
                ImGui::End();

                if (glfwGetMouseButton(window.getGLFWwindow(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
                {
                    showPlanetWindow = true;
                }
            }
        }

        if (showPlanetWindow)
        {
            ImGui::Begin("Planet Info", &showPlanetWindow);
            ImGui::Text("Planet: Earth");
            ImGui::Text("Radius: %.2f", earth.getRadius());
            ImGui::Text("Position: (%.2f, %.2f, %.2f)", earth.getPosition().x, earth.getPosition().y, earth.getPosition().z);
            ImGui::End();
        }

        ImGui::Begin("Solar System");
        ImGui::Text("FPS: %.1f", 1.0f / deltaTime);
        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window.getGLFWwindow());
        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    return 0;
}

void processInput(Window& window, Camera& camera, float deltaTime)
{
    GLFWwindow* glfwWindow = window.getGLFWwindow();
    if (glfwGetKey(glfwWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(glfwWindow, true);

    if (glfwGetKey(glfwWindow, GLFW_KEY_W) == GLFW_PRESS)
        camera.processKeyboard(GLFW_KEY_W, deltaTime);
    if (glfwGetKey(glfwWindow, GLFW_KEY_S) == GLFW_PRESS)
        camera.processKeyboard(GLFW_KEY_S, deltaTime);
    if (glfwGetKey(glfwWindow, GLFW_KEY_A) == GLFW_PRESS)
        camera.processKeyboard(GLFW_KEY_A, deltaTime);
    if (glfwGetKey(glfwWindow, GLFW_KEY_D) == GLFW_PRESS)
        camera.processKeyboard(GLFW_KEY_D, deltaTime);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;
    camera.processMouseMovement(xoffset, yoffset);
}
