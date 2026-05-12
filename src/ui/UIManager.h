#pragma once
#include <string>
#include <vector>
#include "objects/CelestialBody.h"
#include "core/Window.h"
#include "core/Camera.h"
#include "core/Grid.h"
#include "physics/PhysicsSystem.h"

class UIManager {
public:
    void render(Window& window, Camera& camera, float deltaTime,
                std::vector<CelestialBody>& bodies, Grid& grid, PhysicsSystem& physics);
    bool isRightMousePressed(GLFWwindow* window);
    bool isHovered(size_t i) const { return static_cast<int>(i) == hoveredIndex; }
    int  getSelectedPlanetIndex() const { return selectedPlanetIndex; }
    void clearSelection() {
        selectedPlanetIndex = -1;
        lastSelectedIndex   = -1;
        hoveredIndex        = -1;
    }

    // VSync state — driven by the checkbox in the main panel, read by main.cpp
    // after the first render() call to set glfwSwapInterval accordingly.
    bool vsync = true;
    bool vsyncDirty = false;

    // Render-toggle flags, read by main.cpp each frame.
    bool showTrails = true;
    bool showBloom  = true;

private:
    int   selectedPlanetIndex = -1;
    int   hoveredIndex        = -1;
    int   lastSelectedIndex   = -1;
    bool  isMouseMoving       = false;
    float smoothedFps         = 60.0f;

    char      newPlanetName[128] = "New Planet";
    glm::vec3 newPlanetColor{ 0.8f, 0.8f, 0.9f };
    float     newPlanetMass = 1.0e24f;

    char        saveFilename[256] = "savefile.txt";
    std::string saveLoadStatus;
    int         currentPresetIdx = -1;   // -1 = none selected this session

    bool      pendingRemove = false;

    bool      diagBaselineSet  = false;
    double    diagBaselineE    = 0.0;
    double    diagBaselinePmag = 0.0;
    double    diagBaselineLmag = 0.0;

    struct PlanetEditBuffer {
        char       name[128];
        float      mass;
        float      density;
        float      radius;
        glm::vec3  position;
        glm::vec3  velocity;
    } editBuffer;

    void renderPlanetPopup(Window& window, Camera& camera,
                           const glm::mat4& view, const glm::mat4& projection,
                           std::vector<CelestialBody>& bodies);
    void renderPlanetInfo(CelestialBody& body, Camera& camera);
    void renderMainPanel(float deltaTime, std::vector<CelestialBody>& bodies,
                         Grid& grid, PhysicsSystem& physics, Camera& camera);
    void renderNavbar(const std::vector<CelestialBody>& bodies);
};
