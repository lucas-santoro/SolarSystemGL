#include "UIManager.h"
#include "imgui/imgui.h"
#include <cstring>
#include <algorithm>

void UIManager::render(Window& window, Camera& camera, float deltaTime,
    std::vector<std::shared_ptr<Planet>>& planets, Grid& grid) {

    int width, height;
    glfwGetFramebufferSize(window.getGLFWwindow(), &width, &height);
    glm::mat4 view = camera.getViewMatrix();
    float aspect = static_cast<float>(width) / std::max(height, 1);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 20000.0f);

    renderNavbar(planets);
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

        renderPlanetInfo(planet, camera);
    }
}

void UIManager::renderPlanetPopup(Window& window, Camera& camera, const glm::mat4& view, const glm::mat4& projection, const std::vector<std::shared_ptr<Planet>>& planets)
{
    double mouseX, mouseY;
    glfwGetCursorPos(window.getGLFWwindow(), &mouseX, &mouseY);
    int width, height;
    glfwGetFramebufferSize(window.getGLFWwindow(), &width, &height);
    glm::vec3 rayDir = camera.getRayFromMouse(mouseX, mouseY, width, height, view, projection);
    glm::vec3 rayOrig = camera.getPosition();

    if (ImGui::GetIO().WantCaptureMouse) {
        hoveredIndex = -1;
        return;
    }

    hoveredIndex = -1;
    float minDist2 = std::numeric_limits<float>::max();
    for (size_t i = 0; i < planets.size(); ++i) {
        if (planets[i]->intersectsRay(rayOrig, rayDir)) {
            glm::vec3 d = planets[i]->getPosition() - rayOrig;
            float d2 = glm::dot(d, d);
            if (d2 < minDist2) {
                minDist2 = d2;
                hoveredIndex = static_cast<int>(i);
            }
        }
    }
    if (hoveredIndex != -1) {
        glm::vec3 worldAbove = planets[hoveredIndex]->getPosition() + glm::vec3(0.0f, planets[hoveredIndex]->getRadius(), 0.0f);
        glm::vec2 screenPos = camera.worldToScreen(worldAbove, view, projection, width, height);
        if (screenPos.x > 0 && screenPos.y > 0 && screenPos.x < width && screenPos.y < height) {
            ImGui::SetNextWindowPos(ImVec2(screenPos.x, screenPos.y));
            ImGui::Begin("##planet_label", nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::Text("%s", planets[hoveredIndex]->getName().c_str());
            ImGui::End();
        }
    }
    else {
        return;
    }


    if (glfwGetMouseButton(window.getGLFWwindow(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        selectedPlanetIndex = hoveredIndex;
        auto& selectedPlanet = planets[hoveredIndex];

        float distance = std::max(50.0f, selectedPlanet->getRadius() * 4.0f);

        if (camera.getMode() == CameraMode::ORBITAL) {
            camera.setOrbitalTarget(selectedPlanet->getPositionPtr(), distance);
        }
        else {
            camera.startSmoothMove(selectedPlanet->getPosition(), distance);
        }
    }
}

void UIManager::renderPlanetInfo(std::shared_ptr<Planet>& planet, Camera& camera) {
    ImGui::Begin("Planet Info");

    ImGui::InputText("Name", editBuffer.name, sizeof(editBuffer.name));
    ImGui::InputFloat("Mass (kg)", &editBuffer.mass, 0.0f, 0.0f, "%.3e");
    ImGui::InputFloat("Density (kg/m³)", &editBuffer.density);
    ImGui::InputFloat3("Position", &editBuffer.position[0]);
    ImGui::InputFloat3("Velocity", &editBuffer.velocity[0]);

    if (ImGui::Button("Apply Changes")) {
        planet->setName(editBuffer.name);
        planet->setMass(editBuffer.mass);
        planet->setDensity(editBuffer.density);
        planet->setPosition(editBuffer.position);
        planet->setVelocity(editBuffer.velocity);
        planet->recalculateGeometry();
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset")) {
        strncpy_s(editBuffer.name, sizeof(editBuffer.name), planet->getName().c_str(), _TRUNCATE);
        editBuffer.mass = planet->getMass();
        editBuffer.density = planet->getDensity();
        editBuffer.position = planet->getPosition();
        editBuffer.velocity = planet->getVelocity();
    }

    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Camera Mode");
    int currentMode = static_cast<int>(camera.getMode());

    if (camera.getMode() == CameraMode::ORBITAL && camera.getOrbitalTarget() != planet->getPositionPtr()) {
        currentMode = static_cast<int>(CameraMode::FREE);
    }

    if (ImGui::RadioButton("Free", currentMode == static_cast<int>(CameraMode::FREE))) {
        camera.setMode(CameraMode::FREE);
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Orbital", currentMode == static_cast<int>(CameraMode::ORBITAL))) {
        camera.setMode(CameraMode::ORBITAL);
        float distance = std::max(50.0f, planet->getRadius() * 4.0f);
        camera.setOrbitalTarget(planet->getPositionPtr(), distance);
    }

    ImGui::End();
}

void UIManager::renderMainPanel(float deltaTime, std::vector<std::shared_ptr<Planet>>& planets, Grid& grid) {
    ImGui::Begin("Solar System");
    ImGui::Text("FPS: %.1f", 1.0f / deltaTime);
    ImGui::Spacing();
    if (ImGui::Button("Add Planet")) {
        planets.push_back(std::make_shared<Planet>(
            "New Planet", 1.0e24f, 3000.0f,
            glm::vec3(planets.size() * 200.0f, 0.0f, 0.0f),
            glm::vec3(0.0f),
            glm::vec3(0.8f, 0.8f, 0.9f)
        ));
    }
    ImGui::End();
}

bool UIManager::isRightMousePressed(GLFWwindow* window) {
    return !ImGui::GetIO().WantCaptureMouse && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
}

void UIManager::renderNavbar(std::vector<std::shared_ptr<Planet>>& planets) {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Planets")) {
            for (size_t i = 0; i < planets.size(); ++i) {
                if (ImGui::MenuItem(planets[i]->getName().c_str())) {
                    selectedPlanetIndex = static_cast<int>(i);
                }
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}