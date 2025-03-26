#include <glad/glad.h>
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"

#include "core/Window.h"
#include "core/Shader.h"
#include "objects/Planet.h"
#include "core/Camera.h"
#include "core/Grid.h"
#include <memory>

void processInput(Window& window, Camera& camera, float deltaTime);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
bool showPlanetWindow = false;
bool isMouseMoving = false;
bool isMouseRotating = false;
float lastX = 400, lastY = 300;
bool firstMouse = true;
float deltaTime = 0.0f;
float lastFrame = 0.0f;
int selectedPlanetIndex = -1;

int main()
{
    Window window(800, 600, "SolarSystemGL");
    glfwSetCursorPosCallback(window.getGLFWwindow(), mouse_callback);
    glfwSetScrollCallback(window.getGLFWwindow(), scroll_callback);
    glfwSetInputMode(window.getGLFWwindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glEnable(GL_DEPTH_TEST);

    Shader shader("shaders/VertexShader.glsl", "shaders/FragmentShader.glsl");
	Shader gridShader("shaders/GridVertexShader.glsl", "shaders/GridFragmentShader.glsl");

    std::vector<std::shared_ptr<Planet>> planets;
    float AU = 3.0f;

    //planets.push_back(std::make_shared<Planet>("Sun", 1.989e30f, 1408.0f, glm::vec3(0.0f), glm::vec3(0.0f)));
    planets.push_back(std::make_shared<Planet>("Mercury", 3.3011e23f, 5427.0f, glm::vec3(0.387f * AU, 0.0f, 0.0f), glm::vec3(0.0f)));
    planets.push_back(std::make_shared<Planet>("Venus", 4.8675e24f, 5243.0f, glm::vec3(0.723f * AU, 0.0f, 0.0f), glm::vec3(0.0f)));
    planets.push_back(std::make_shared<Planet>("Earth", 5.972e24f, 5514.0f, glm::vec3(1.0f * AU, 0.0f, 0.0f), glm::vec3(0.0f)));
    planets.push_back(std::make_shared<Planet>("Moon", 7.3477e22f, 3344.0f, glm::vec3(1.0f * AU + 0.00257f * AU, 0.0f, 0.0f), glm::vec3(0.0f)));
    planets.push_back(std::make_shared<Planet>("Mars", 6.417e23f, 3933.0f, glm::vec3(1.524f * AU, 0.0f, 0.0f), glm::vec3(0.0f)));
    planets.push_back(std::make_shared<Planet>("Jupiter", 1.898e27f, 1326.0f, glm::vec3(5.203f * AU, 0.0f, 0.0f), glm::vec3(0.0f)));
    planets.push_back(std::make_shared<Planet>("Saturn", 5.683e26f, 687.0f, glm::vec3(9.537f * AU, 0.0f, 0.0f), glm::vec3(0.0f)));
    planets.push_back(std::make_shared<Planet>("Uranus", 8.681e25f, 1271.0f, glm::vec3(19.191f * AU, 0.0f, 0.0f), glm::vec3(0.0f)));
    planets.push_back(std::make_shared<Planet>("Neptune", 1.024e26f, 1638.0f, glm::vec3(30.07f * AU, 0.0f, 0.0f), glm::vec3(0.0f)));


    Grid grid(1000.0f, 300, 0.0f);
    if (!planets.empty()) {
        grid.applyGravityDistortion(planets);
    }

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

        for (const auto& planet : planets) {
            planet->render(shader);
        }

		gridShader.use();
        gridShader.setMat4("view", view);
        gridShader.setMat4("projection", projection);
        gridShader.setMat4("model", glm::mat4(1.0f));
        grid.draw();

        double mouseX, mouseY;
        glfwGetCursorPos(window.getGLFWwindow(), &mouseX, &mouseY);

        glm::vec3 rayDirection = camera.getRayFromMouse(mouseX, mouseY, 800, 600, view, projection);
        glm::vec3 rayOrigin = camera.getPosition();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        bool isMouseOverUI = ImGui::GetIO().WantCaptureMouse;

        if (!isMouseOverUI)
        {
            if (glfwGetMouseButton(window.getGLFWwindow(), GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
            {
                isMouseMoving = true;
            }
            else if (glfwGetMouseButton(window.getGLFWwindow(), GLFW_MOUSE_BUTTON_RIGHT) == GLFW_RELEASE)
            {
                isMouseMoving = false;
            }
        }

        for (size_t i = 0; i < planets.size(); i++)
        {
            if (planets[i]->intersectsRay(camera.getPosition(), rayDirection))
            {
                glm::vec3 planetScreenPos = planets[i]->getPosition() + glm::vec3(0.0f, planets[i]->getRadius() + 0.2f, 0.0f);
                glm::vec2 screenPos = camera.worldToScreen(planetScreenPos, view, projection, 800, 600);

                if (screenPos.x >= 0 && screenPos.y >= 0 && screenPos.x <= 800 && screenPos.y <= 600)
                {
                    ImGui::SetNextWindowPos(ImVec2(screenPos.x, screenPos.y));
                    ImGui::Begin("##PlanetName", nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
                    ImGui::Text("%s", planets[i]->getName().c_str());
                    ImGui::End();

                    if (glfwGetMouseButton(window.getGLFWwindow(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
                    {
                        showPlanetWindow = true;
                        selectedPlanetIndex = i;
                    }
                }
            }
        }

        if (showPlanetWindow && selectedPlanetIndex >= 0 && selectedPlanetIndex < planets.size())
        {
            auto& planet = planets[selectedPlanetIndex];

            ImGui::Begin("Planet Info", &showPlanetWindow);

            static char nameBuffer[128];
            strncpy_s(nameBuffer, sizeof(nameBuffer), planet->getName().c_str(), _TRUNCATE);
            nameBuffer[sizeof(nameBuffer) - 1] = '\0';

            ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer));
            static float mass = planet->getMass();
            static float density = planet->getDensity();
            static float radius = planet->getRadius() * 1e7f;
            static glm::vec3 position = planet->getPosition();
            static glm::vec3 velocity = planet->getVelocity();

            ImGui::InputFloat("Mass (kg)", &mass, 0.0f, 0.0f, "%.3e");
            ImGui::InputFloat("Density (kg/m³)", &density);
            ImGui::InputFloat("Radius (m)", &radius);
            ImGui::InputFloat3("Position", &position[0]);
            ImGui::InputFloat3("Velocity", &velocity[0]);

            if (ImGui::Button("Reset"))
            {
                std::cout << "reset button clicked";
            }
            ImGui::SameLine();

            if (ImGui::Button("Apply"))
            {
                std::cout << "apply button clicked";
            }

            ImGui::End();
        }

        ImGui::Begin("Solar System");
        ImGui::Text("FPS: %.1f", 1.0f / deltaTime);

        if (ImGui::Button("Add Planet"))
        {
            planets.push_back(std::make_shared<Planet>("New Planet", 1.0e24f, 3000.0f,
                glm::vec3(planets.size() * 2.0f, 0.0f, 0.0f),
                glm::vec3(0.0f)));
            grid.applyGravityDistortion(planets);
        }

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
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    if (isMouseMoving)
    {
        camera.processMouseMovement(xoffset, yoffset);
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (!ImGui::GetIO().WantCaptureMouse)
    {
        camera.processKeyboard(yoffset > 0 ? GLFW_KEY_W : GLFW_KEY_S, 0.1f);
    }
}