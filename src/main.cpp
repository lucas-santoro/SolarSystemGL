#include <glad/glad.h>
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"

#include "core/Window.h"
#include "core/Shader.h"
#include "objects/Planet.h"
#include "core/Camera.h"
#include "core/Grid.h"
#include "ui/UIManager.h"
#include "core/Constants.h"
#include "physics/BodyState.h"

#include <memory>

void processInput(Window& window, Camera& camera, float deltaTime);
void mouseCallback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

Camera camera(glm::vec3(0.0f, 0.0f, 300.0f));
float lastX = 400, lastY = 300;
bool firstMouse = true;
bool isMouseMoving = false;
float deltaTime = 0.0f;
float lastFrame = 0.0f;

UIManager uiManager;

int main()
{
    Window window(800, 600, "SolarSystemGL");
    glfwSetFramebufferSizeCallback(window.getGLFWwindow(), [](GLFWwindow*, int width, int height) { glViewport(0, 0, width, height); });
    glfwSetCursorPosCallback(window.getGLFWwindow(), mouseCallback);
    glfwSetScrollCallback(window.getGLFWwindow(), scroll_callback);
    glfwSetInputMode(window.getGLFWwindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Shader shader("shaders/VertexShader.glsl", "shaders/FragmentShader.glsl");
	Shader gridShader("shaders/GridVertexShader.glsl", "shaders/GridFragmentShader.glsl");

    std::vector<std::shared_ptr<Planet>> planets;
    std::vector<BodyState> bodies;

    constexpr double METERS_PER_WU = 1.0e9;
    constexpr double AU = 1.495978707e11;
    constexpr float  AU_WU = static_cast<float>(AU / METERS_PER_WU);

    planets.emplace_back(std::make_shared<Planet>(
        "Sun", 1.989e30f, 1408.0f,
        glm::vec3(0.0f), glm::vec3(0.0f),
        glm::vec3(1.0f, 0.9f, 0.3f)));

    planets.emplace_back(std::make_shared<Planet>(
        "Mercury", 3.3011e23f, 5427.0f,
        glm::vec3(0.387f * AU_WU, 0.0f, 0.0f), glm::vec3(0.0f),
        glm::vec3(0.5f, 0.5f, 0.5f)));

    planets.emplace_back(std::make_shared<Planet>(
        "Venus", 4.8675e24f, 5243.0f,
        glm::vec3(0.723f * AU_WU, 0.0f, 0.0f), glm::vec3(0.0f),
        glm::vec3(0.95f, 0.85f, 0.55f)));

    planets.emplace_back(std::make_shared<Planet>(
        "Earth", 5.972e24f, 5514.0f,
        glm::vec3(1.0f * AU_WU, 0.0f, 0.0f), glm::vec3(0.0f),
        glm::vec3(0.2f, 0.4f, 1.0f)));

    planets.emplace_back(std::make_shared<Planet>(
        "Mars", 6.417e23f, 3933.0f,
        glm::vec3(1.524f * AU_WU, 0.0f, 0.0f), glm::vec3(0.0f),
        glm::vec3(0.8f, 0.3f, 0.1f)));

    planets.emplace_back(std::make_shared<Planet>(
        "Jupiter", 1.898e27f, 1326.0f,
        glm::vec3(5.203f * AU_WU, 0.0f, 0.0f), glm::vec3(0.0f),
        glm::vec3(0.9f, 0.7f, 0.4f)));

    planets.emplace_back(std::make_shared<Planet>(
        "Saturn", 5.683e26f, 687.0f,
        glm::vec3(9.537f * AU_WU, 0.0f, 0.0f), glm::vec3(0.0f),
        glm::vec3(0.95f, 0.85f, 0.5f)));

    planets.emplace_back(std::make_shared<Planet>(
        "Uranus", 8.681e25f, 1271.0f,
        glm::vec3(19.191f * AU_WU, 0.0f, 0.0f), glm::vec3(0.0f),
        glm::vec3(0.6f, 0.85f, 0.9f)));

    planets.emplace_back(std::make_shared<Planet>(
        "Neptune", 1.024e26f, 1638.0f,
        glm::vec3(30.07f * AU_WU, 0.0f, 0.0f), glm::vec3(0.0f),
        glm::vec3(0.3f, 0.4f, 0.85f)));

    auto addBody = [&](double a_au, double v, double m)
        {
            bodies.push_back({
                glm::dvec3(a_au * AU, 0.0, 0.0),
                glm::dvec3(0.0, 0.0, v),
                m });
        };

    addBody(0.000, 0.0, 1.989e30); // Sol
    addBody(0.387, 47900.0, 3.3011e23); // Mercúrio
    addBody(0.723, 35000.0, 4.8675e24); // Vênus
    addBody(1.000, 29780.0, 5.9720e24); // Terra
    addBody(1.524, 24100.0, 6.4171e23); // Marte
    addBody(5.203, 13070.0, 1.8980e27); // Júpiter
    addBody(9.537, 9680.0, 5.6834e26); // Saturno
    addBody(19.191, 6800.0, 8.6810e25); // Urano
    addBody(30.070, 5430.0, 1.0240e26); // Netuno

    Grid grid(1000.0f, 300, 0.0f);

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

        int width, height;
        glfwGetFramebufferSize(window.getGLFWwindow(), &width, &height);
        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)width / std::max(height, 1), 0.01f, 1000.0f);
        glm::mat4 model = glm::mat4(1.0f);

        shader.setMat4("view", view);
        shader.setMat4("projection", projection);
        shader.setMat4("model", model);

        constexpr double TIME_SCALE = 360000.0;
        double dtSim = static_cast<double>(deltaTime) * TIME_SCALE;
        constexpr double G = 6.67430e-11;
        constexpr double SOFTEN = 1e3;
         
        for (size_t i = 0; i < bodies.size(); ++i)
        {
            glm::dvec3 acc_m = glm::dvec3(0.0);

            for (size_t j = 0; j < bodies.size(); ++j)
            {
                if (i == j) continue;

                glm::dvec3 r = bodies[j].pos_m - bodies[i].pos_m;
                double dist2 = glm::dot(r, r) + SOFTEN;
                double invDist = 1.0 / sqrt(dist2);
                acc_m += (G * bodies[j].mass_kg * invDist * invDist) * r * invDist;
            }

            bodies[i].vel_m += acc_m * dtSim;
        }

        for (auto& b : bodies)
        {
            b.pos_m += b.vel_m * dtSim;
        }


        for (size_t i = 0; i < planets.size(); ++i)
        {
            glm::vec3 renderPos = glm::vec3(bodies[i].pos_m / METERS_PER_WU);
            planets[i]->setPosition(renderPos);
        }

        for (size_t i = 0; i < planets.size(); ++i)
            planets[i]->render(shader, uiManager.isHovered(i));


        gridShader.use();
        gridShader.setMat4("view", view);
        gridShader.setMat4("projection", projection);
        gridShader.setMat4("model", glm::mat4(1.0f));
        grid.draw(gridShader, planets);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        uiManager.render(window, camera, deltaTime, planets, grid);

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

void mouseCallback(GLFWwindow* window, double xpos, double ypos)
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

    if (uiManager.isRightMousePressed(window)) {
        camera.processMouseMovement(xoffset, yoffset);
    }
}


void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (!ImGui::GetIO().WantCaptureMouse)
        camera.processKeyboard(yoffset > 0 ? GLFW_KEY_W : GLFW_KEY_S, 0.1f);
}
