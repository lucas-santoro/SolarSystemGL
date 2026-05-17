#include "UIManager.h"
#include "imgui/imgui.h"
#include "core/Constants.h"
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <limits>

void UIManager::render(Window& window, Camera& camera, float deltaTime,
                       std::vector<CelestialBody>& bodies, Grid& /*grid*/, PhysicsSystem& physics)
{
    int width, height;
    glfwGetFramebufferSize(window.getGLFWwindow(), &width, &height);
    glm::mat4 view = camera.getViewMatrix();
    float aspect = static_cast<float>(width) / static_cast<float>(std::max(height, 1));
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 20000.0f);

    actionbar_.renderTopBar(window, camera, physics, bodies, *this, deltaTime);
    renderPlanetPopup(window, camera, view, projection, bodies);

    if (selectedPlanetIndex >= 0 && selectedPlanetIndex < static_cast<int>(bodies.size()))
    {
        auto& body = bodies[selectedPlanetIndex];

        if (selectedPlanetIndex != lastSelectedIndex)
        {
            std::snprintf(editBuffer.name, sizeof(editBuffer.name), "%s", body.name.c_str());
            editBuffer.mass     = static_cast<float>(body.mass_kg);
            editBuffer.density  = body.density;
            editBuffer.position = glm::vec3(body.pos_m / METERS_PER_WU);
            editBuffer.velocity = glm::vec3(body.vel_m / 1000.0);
            lastSelectedIndex = selectedPlanetIndex;
        }

        renderPlanetInfo(body);
    }

    if (showDiagnostics) renderDiagnostics(bodies);

    actionbar_.renderBottomBar(window, *this);

    // Deferred removal — set inside renderPlanetInfo; processed here so we don't
    // mutate the bodies vector while the panel still holds a reference to it.
    if (pendingRemove
        && selectedPlanetIndex >= 0
        && selectedPlanetIndex < static_cast<int>(bodies.size()))
    {
        const std::string removedName = bodies[selectedPlanetIndex].name;
        camera.shiftOrbitalIndexOnRemove(selectedPlanetIndex);
        bodies.erase(bodies.begin() + selectedPlanetIndex);
        selectedPlanetIndex = -1;
        lastSelectedIndex   = -1;
        hoveredIndex        = -1;
        diagBaselineSet     = false;  // body count changed → invalidate baseline
        toasts_.info("Removed " + removedName);
    }
    pendingRemove = false;

    toasts_.update(deltaTime);
    toasts_.render();
}

void UIManager::renderPlanetPopup(Window& window, Camera& camera,
                                  const glm::mat4& view, const glm::mat4& projection,
                                  std::vector<CelestialBody>& bodies)
{
    double mouseX, mouseY;
    glfwGetCursorPos(window.getGLFWwindow(), &mouseX, &mouseY);
    int width, height;
    glfwGetFramebufferSize(window.getGLFWwindow(), &width, &height);
    glm::vec3 rayDir  = camera.getRayFromMouse(mouseX, mouseY, width, height, view, projection);
    glm::vec3 rayOrig = camera.getPosition();

    if (ImGui::GetIO().WantCaptureMouse) {
        hoveredIndex = -1;
        return;
    }

    hoveredIndex = -1;
    float minDist2 = std::numeric_limits<float>::max();
    for (size_t i = 0; i < bodies.size(); ++i) {
        if (bodies[i].intersectsRay(rayOrig, rayDir)) {
            glm::vec3 d = bodies[i].renderPosition() - rayOrig;
            float d2 = glm::dot(d, d);
            if (d2 < minDist2) {
                minDist2 = d2;
                hoveredIndex = static_cast<int>(i);
            }
        }
    }
    if (hoveredIndex != -1) {
        glm::vec3 worldAbove = bodies[hoveredIndex].renderPosition()
                              + glm::vec3(0.0f, bodies[hoveredIndex].radius, 0.0f);
        glm::vec2 screenPos = camera.worldToScreen(worldAbove, view, projection, width, height);
        if (screenPos.x > 0 && screenPos.y > 0 && screenPos.x < width && screenPos.y < height) {
            ImGui::SetNextWindowPos(ImVec2(screenPos.x, screenPos.y));
            ImGui::Begin("##planet_label", nullptr,
                         ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar |
                         ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::Text("%s", bodies[hoveredIndex].name.c_str());
            ImGui::End();
        }
    }
    else {
        return;
    }

    if (glfwGetMouseButton(window.getGLFWwindow(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        selectedPlanetIndex = hoveredIndex;
        auto& selected = bodies[hoveredIndex];

        const float distance = selected.focusDistance();

        if (camera.getMode() == CameraMode::ORBITAL) {
            camera.flyToOrbital(hoveredIndex, selected.renderPosition(), distance);
        }
        else {
            camera.startSmoothMove(selected.renderPosition(), distance);
        }
    }
}

void UIManager::renderPlanetInfo(CelestialBody& body)
{
    ImGui::Begin("Planet Info");

    ImGui::InputText("Name", editBuffer.name, sizeof(editBuffer.name));
    ImGui::InputFloat("Mass (kg)", &editBuffer.mass, 0.0f, 0.0f, "%.3e");
    ImGui::InputFloat("Density (kg/m^3)", &editBuffer.density);
    ImGui::InputFloat3("Position (WU)", &editBuffer.position[0]);
    ImGui::InputFloat3("Velocity (km/s)", &editBuffer.velocity[0]);

    if (ImGui::Button("Apply Changes")) {
        body.name     = editBuffer.name;
        body.mass_kg  = static_cast<double>(editBuffer.mass);
        body.density  = editBuffer.density;
        body.pos_m    = glm::dvec3(editBuffer.position) * METERS_PER_WU;
        body.vel_m    = glm::dvec3(editBuffer.velocity) * 1000.0;
        body.recalculateGeometry();
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset")) {
        std::snprintf(editBuffer.name, sizeof(editBuffer.name), "%s", body.name.c_str());
        editBuffer.mass     = static_cast<float>(body.mass_kg);
        editBuffer.density  = body.density;
        editBuffer.position = glm::vec3(body.pos_m / METERS_PER_WU);
        editBuffer.velocity = glm::vec3(body.vel_m / 1000.0);
    }
    ImGui::SameLine();
    if (ImGui::Button("Remove")) {
        pendingRemove = true;
    }

    ImGui::End();
}

bool UIManager::isRightMousePressed(GLFWwindow* window) {
    return !ImGui::GetIO().WantCaptureMouse
           && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
}

void UIManager::renderDiagnostics(const std::vector<CelestialBody>& bodies)
{
    ImGui::Begin("Diagnostics", &showDiagnostics);

    double KE = 0.0, PE = 0.0;
    glm::dvec3 p_total(0.0);
    glm::dvec3 L_total(0.0);
    for (size_t i = 0; i < bodies.size(); ++i) {
        const auto& bi = bodies[i];
        p_total += bi.vel_m * bi.mass_kg;
        L_total += bi.mass_kg * glm::cross(bi.pos_m, bi.vel_m);
        KE      += 0.5 * bi.mass_kg * glm::dot(bi.vel_m, bi.vel_m);
        for (size_t j = i + 1; j < bodies.size(); ++j) {
            const auto& bj    = bodies[j];
            const glm::dvec3 r = bj.pos_m - bi.pos_m;
            const double dist = std::sqrt(glm::dot(r, r) + PhysicsSystem::SOFTEN);
            PE -= PhysicsSystem::G * bi.mass_kg * bj.mass_kg / dist;
        }
    }
    const double E    = KE + PE;
    const double pmag = glm::length(p_total);
    const double Lmag = glm::length(L_total);

    if (!diagBaselineSet) {
        diagBaselineE    = E;
        diagBaselinePmag = pmag;
        diagBaselineLmag = Lmag;
        diagBaselineSet  = true;
    }

    ImGui::Text("KE      %.4e J", KE);
    ImGui::Text("PE      %.4e J", PE);
    ImGui::Text("E       %.4e J  (drift %.2e)", E,
                diagBaselineE != 0.0 ? (E - diagBaselineE) / std::abs(diagBaselineE) : 0.0);
    ImGui::Text("|p|     %.4e  (drift %.2e)", pmag,
                diagBaselinePmag != 0.0 ? (pmag - diagBaselinePmag) / diagBaselinePmag : 0.0);
    ImGui::Text("|L|     %.4e  (drift %.2e)", Lmag,
                diagBaselineLmag != 0.0 ? (Lmag - diagBaselineLmag) / diagBaselineLmag : 0.0);

    if (ImGui::SmallButton("Reset baseline")) {
        diagBaselineSet = false;
    }

    ImGui::End();
}
