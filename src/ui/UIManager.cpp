#include "UIManager.h"
#include "imgui/imgui.h"
#include "core/Constants.h"
#include "core/SaveLoad.h"
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <limits>

void UIManager::render(Window& window, Camera& camera, float deltaTime,
                       std::vector<CelestialBody>& bodies, Grid& grid, PhysicsSystem& physics)
{
    int width, height;
    glfwGetFramebufferSize(window.getGLFWwindow(), &width, &height);
    glm::mat4 view = camera.getViewMatrix();
    float aspect = static_cast<float>(width) / static_cast<float>(std::max(height, 1));
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 20000.0f);

    renderNavbar(bodies);
    renderPlanetPopup(window, camera, view, projection, bodies);
    renderMainPanel(deltaTime, bodies, grid, physics, camera);

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

        renderPlanetInfo(body, camera);
    }

    // Deferred removal — set inside renderPlanetInfo; processed here so we don't
    // mutate the bodies vector while the panel still holds a reference to it.
    if (pendingRemove
        && selectedPlanetIndex >= 0
        && selectedPlanetIndex < static_cast<int>(bodies.size()))
    {
        camera.shiftOrbitalIndexOnRemove(selectedPlanetIndex);
        bodies.erase(bodies.begin() + selectedPlanetIndex);
        selectedPlanetIndex = -1;
        lastSelectedIndex   = -1;
        hoveredIndex        = -1;
        diagBaselineSet     = false;  // body count changed → invalidate baseline
    }
    pendingRemove = false;
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

void UIManager::renderPlanetInfo(CelestialBody& body, Camera& camera)
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

    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Camera Mode");
    int currentMode = static_cast<int>(camera.getMode());

    if (camera.getMode() == CameraMode::ORBITAL
        && camera.getOrbitalTargetIndex() != selectedPlanetIndex) {
        currentMode = static_cast<int>(CameraMode::FREE);
    }

    if (ImGui::RadioButton("Free", currentMode == static_cast<int>(CameraMode::FREE))) {
        camera.setMode(CameraMode::FREE);
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Orbital", currentMode == static_cast<int>(CameraMode::ORBITAL))) {
        camera.flyToOrbital(selectedPlanetIndex, body.renderPosition(), body.focusDistance());
    }

    ImGui::End();
}

void UIManager::renderMainPanel(float deltaTime, std::vector<CelestialBody>& bodies,
                                Grid& /*grid*/, PhysicsSystem& physics, Camera& camera)
{
    ImGui::Begin("Solar System");

    const float instantFps = 1.0f / std::max(deltaTime, 1e-6f);
    smoothedFps = smoothedFps * 0.9f + instantFps * 0.1f;
    ImGui::Text("FPS: %.1f", smoothedFps);
    if (ImGui::Checkbox("VSync", &vsync)) {
        vsyncDirty = true;
    }

    ImGui::Separator();
    ImGui::Checkbox("Trails", &showTrails);
    ImGui::SameLine();
    ImGui::Checkbox("Bloom",  &showBloom);

    ImGui::Separator();
    ImGui::Checkbox("Paused", &physics.paused);
    ImGui::SliderFloat("Time scale", &physics.timeScale,
                       1.0f, 5.0e6f, "%.0fx", ImGuiSliderFlags_Logarithmic);
    if (ImGui::SmallButton("1x"))    physics.timeScale = 1.0f;
    ImGui::SameLine();
    if (ImGui::SmallButton("1k x"))  physics.timeScale = 1000.0f;
    ImGui::SameLine();
    if (ImGui::SmallButton("100k x")) physics.timeScale = 100000.0f;
    ImGui::SameLine();
    if (ImGui::SmallButton("1M x"))  physics.timeScale = 1.0e6f;

    ImGui::Separator();
    if (ImGui::Button("Reset Camera")) {
        camera.reset();
    }

    if (ImGui::CollapsingHeader("Diagnostics")) {
        double KE = 0.0, PE = 0.0;
        glm::dvec3 p_total(0.0);
        glm::dvec3 L_total(0.0);
        for (size_t i = 0; i < bodies.size(); ++i) {
            const auto& bi = bodies[i];
            p_total += bi.vel_m * bi.mass_kg;
            L_total += bi.mass_kg * glm::cross(bi.pos_m, bi.vel_m);
            KE      += 0.5 * bi.mass_kg * glm::dot(bi.vel_m, bi.vel_m);
            for (size_t j = i + 1; j < bodies.size(); ++j) {
                const auto& bj   = bodies[j];
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
    }

    ImGui::Separator();
    {
        // Preset bundle — points to text files copied next to the exe by CMake POST_BUILD.
        struct Preset { const char* label; const char* path; };
        static const Preset presets[] = {
            { "Solar System (default)", "presets/solar-system.txt" },
            { "Sun only (blank slate)", "presets/sun-only.txt" },
            { "Binary stars",           "presets/binary-stars.txt" },
        };
        constexpr int presetCount = static_cast<int>(sizeof(presets) / sizeof(presets[0]));

        const char* previewLabel = (currentPresetIdx >= 0 && currentPresetIdx < presetCount)
                                       ? presets[currentPresetIdx].label
                                       : "Choose preset...";
        ImGui::Text("Presets");
        if (ImGui::BeginCombo("##preset", previewLabel)) {
            for (int i = 0; i < presetCount; ++i) {
                const bool selected = (i == currentPresetIdx);
                if (ImGui::Selectable(presets[i].label, selected)) {
                    if (loadSimulation(presets[i].path, bodies, physics)) {
                        currentPresetIdx    = i;
                        camera.setMode(CameraMode::FREE);
                        selectedPlanetIndex = -1;
                        lastSelectedIndex   = -1;
                        hoveredIndex        = -1;
                        diagBaselineSet     = false;
                        saveLoadStatus      = std::string("Loaded preset: ") + presets[i].label;
                    } else {
                        saveLoadStatus = std::string("Failed to load preset: ") + presets[i].path;
                    }
                }
                if (selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
    }

    ImGui::Separator();
    ImGui::Text("Save / Load");
    ImGui::InputText("File##saveload", saveFilename, sizeof(saveFilename));
    if (ImGui::Button("Save")) {
        if (saveSimulation(saveFilename, bodies, physics))
            saveLoadStatus = std::string("Saved to ") + saveFilename;
        else
            saveLoadStatus = std::string("Save failed: cannot write ") + saveFilename;
    }
    ImGui::SameLine();
    if (ImGui::Button("Load")) {
        if (loadSimulation(saveFilename, bodies, physics)) {
            // Body vector replaced — invalidate everything that referenced indices into it.
            camera.setMode(CameraMode::FREE);
            selectedPlanetIndex = -1;
            lastSelectedIndex   = -1;
            hoveredIndex        = -1;
            diagBaselineSet     = false;
            saveLoadStatus      = std::string("Loaded from ") + saveFilename;
        } else {
            saveLoadStatus = std::string("Load failed: cannot open ") + saveFilename;
        }
    }
    if (!saveLoadStatus.empty()) {
        ImGui::TextWrapped("%s", saveLoadStatus.c_str());
    }

    ImGui::Separator();
    ImGui::Text("New body:");
    ImGui::InputText("Name##new", newPlanetName, sizeof(newPlanetName));
    ImGui::ColorEdit3("Color##new", &newPlanetColor[0]);
    ImGui::InputFloat("Mass (kg)##new", &newPlanetMass, 0.0f, 0.0f, "%.3e");
    if (ImGui::Button("Add Planet")) {
        constexpr double SUN_MASS = 1.989e30;
        const double a     = 1.5 * AU;
        const double angle = static_cast<double>(bodies.size()) * 0.7;
        const double v     = std::sqrt(PhysicsSystem::G * SUN_MASS / a);

        CelestialBody nb;
        nb.pos_m   = glm::dvec3(a * std::cos(angle), 0.0, a * std::sin(angle));
        nb.vel_m   = glm::dvec3(-v * std::sin(angle), 0.0, v * std::cos(angle));
        nb.mass_kg = static_cast<double>(newPlanetMass);
        nb.name    = newPlanetName;
        nb.color   = newPlanetColor;
        nb.density = 3000.0f;
        nb.recalculateGeometry();
        bodies.push_back(std::move(nb));
        selectedPlanetIndex = static_cast<int>(bodies.size()) - 1;
    }
    ImGui::End();
}

bool UIManager::isRightMousePressed(GLFWwindow* window) {
    return !ImGui::GetIO().WantCaptureMouse
           && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
}

void UIManager::renderNavbar(const std::vector<CelestialBody>& bodies)
{
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Planets")) {
            for (size_t i = 0; i < bodies.size(); ++i) {
                if (ImGui::MenuItem(bodies[i].name.c_str())) {
                    selectedPlanetIndex = static_cast<int>(i);
                }
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}
