#include "UIManager.h"
#include "imgui/imgui.h"
#include <cstring>

void UIManager::render(Window& window, Camera& camera, float deltaTime,
    std::vector<std::shared_ptr<Planet>>& planets, Grid& grid) {
    int width, height;
    glfwGetFramebufferSize(window.getGLFWwindow(), &width, &height);
    glm::mat4 view = camera.getViewMatrix();
    float aspect = (float)width / std::max(height, 1);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);

    renderPlanetPopup(window, camera, view, projection, planets);
    renderMainPanel(deltaTime, planets, grid);

    if (selectedPlanetIndex >= 0 && selectedPlanetIndex < planets.size()) {
        auto& planet = planets[selectedPlanetIndex];

        if (selectedPlanetIndex != lastSelectedIndex) {
            strncpy_s(editBuffer.name, sizeof(editBuffer.name), planet->getName().c_str(), _TRUNCATE);
            editBuffer.mass = planet->getMass();
            editBuffer.density = planet->getDensity();
            editBuffer.position = planet->getPosition();
            editBuffer.velocity = planet->getVelocity();
            lastSelectedIndex = selectedPlanetIndex;
        }

        renderPlanetInfo(planet);
    }
}

void UIManager::renderPlanetPopup(Window& window, Camera& camera, const glm::mat4& view, const glm::mat4& projection,
    const std::vector<std::shared_ptr<Planet>>& planets) {
    double mouseX, mouseY;
    glfwGetCursorPos(window.getGLFWwindow(), &mouseX, &mouseY);

    int width, height;
    glfwGetFramebufferSize(window.getGLFWwindow(), &width, &height);
    glm::vec3 rayDir = camera.getRayFromMouse(mouseX, mouseY, width, height, view, projection);
    glm::vec3 rayOrigin = camera.getPosition();

    bool isMouseOverUI = ImGui::GetIO().WantCaptureMouse;

    if (!isMouseOverUI) {
        if (glfwGetMouseButton(window.getGLFWwindow(), GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
            isMouseMoving = true;
        else if (glfwGetMouseButton(window.getGLFWwindow(), GLFW_MOUSE_BUTTON_RIGHT) == GLFW_RELEASE)
            isMouseMoving = false;
    }

    for (size_t i = 0; i < planets.size(); i++) {
        if (planets[i]->intersectsRay(rayOrigin, rayDir)) {
            glm::vec3 screenWorldPos = planets[i]->getPosition() + glm::vec3(0.0f, planets[i]->getRadius() + 0.2f, 0.0f);
            glm::vec2 screenPos = camera.worldToScreen(screenWorldPos, view, projection, width, height);

            if (screenPos.x >= 0 && screenPos.y >= 0 && screenPos.x <= width && screenPos.y <= height) {
                ImGui::SetNextWindowPos(ImVec2(screenPos.x, screenPos.y));
                ImGui::Begin("##PlanetName", nullptr,
                    ImGuiWindowFlags_NoBackground |
                    ImGuiWindowFlags_NoTitleBar |
                    ImGuiWindowFlags_NoResize);
                ImGui::Text("%s", planets[i]->getName().c_str());
                ImGui::End();

                if (glfwGetMouseButton(window.getGLFWwindow(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
                    selectedPlanetIndex = static_cast<int>(i);
                }
            }
        }
    }
}

void UIManager::renderPlanetInfo(std::shared_ptr<Planet>& planet) {
    ImGui::Begin("Planet Info");

    ImGui::InputText("Name", editBuffer.name, sizeof(editBuffer.name));
    ImGui::InputFloat("Mass (kg)", &editBuffer.mass, 0.0f, 0.0f, "%.3e");
    ImGui::InputFloat("Density (kg/m³)", &editBuffer.density);
    ImGui::InputFloat3("Position", &editBuffer.position[0]);
    ImGui::InputFloat3("Velocity", &editBuffer.velocity[0]);

    if (ImGui::Button("Reset")) {
        editBuffer.mass = planet->getMass();
        editBuffer.density = planet->getDensity();
        editBuffer.position = planet->getPosition();
        editBuffer.velocity = planet->getVelocity();
    }

    ImGui::SameLine();

    if (ImGui::Button("Apply")) {
        planet->setName(editBuffer.name);
        planet->setMass(editBuffer.mass);
        planet->setDensity(editBuffer.density);
        planet->setPosition(editBuffer.position);
        planet->setVelocity(editBuffer.velocity);
        planet->recalculateGeometry();
    }

    ImGui::End();
}

void UIManager::renderMainPanel(float deltaTime, std::vector<std::shared_ptr<Planet>>& planets, Grid& grid) {
    ImGui::Begin("Solar System");
    ImGui::Text("FPS: %.1f", 1.0f / deltaTime);

    if (ImGui::Button("Add Planet")) 
    {
        planets.push_back(std::make_shared<Planet>(
            "New Planet", 1.0e24f, 3000.0f,
            glm::vec3(planets.size() * 2.0f, 0.0f, 0.0f),
            glm::vec3(0.0f),
            glm::vec3(1.0f, 1.0f, 1.0f)
        ));
        grid.applyGravityDistortion(planets);
    }

    ImGui::End();
}

bool UIManager::isRightMousePressed(GLFWwindow *window) 
{
    return !ImGui::GetIO().WantCaptureMouse && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
}
