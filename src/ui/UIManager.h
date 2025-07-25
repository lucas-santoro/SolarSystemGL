#pragma once
#include <vector>
#include <memory>
#include "objects/Planet.h"
#include "core/Window.h"
#include "core/Camera.h"
#include "core/Grid.h"

class UIManager {
public:
    void render(Window &window, Camera &camera, float deltaTime,
        std::vector<std::shared_ptr<Planet>>& planets, Grid& grid);
    bool isRightMousePressed(GLFWwindow *window);
    bool isHovered(size_t i) const { return static_cast<int>(i) == hoveredIndex; }

private:
    int selectedPlanetIndex = -1;
    int hoveredIndex = -1;
    int lastSelectedIndex = -1;
    bool isMouseMoving = false;

    struct PlanetEditBuffer {
        char name[128];
        float mass;
        float density;
        float radius;
        glm::vec3 position;
        glm::vec3 velocity;
    } editBuffer;

    void renderPlanetPopup(Window &window, Camera &camera, const glm::mat4 &view, const glm::mat4 &projection,
        const std::vector<std::shared_ptr<Planet>> &planets);
    void renderPlanetInfo(std::shared_ptr<Planet> &planet, Camera &camera);
    void renderMainPanel(float deltaTime, std::vector<std::shared_ptr<Planet>> &planets, Grid &grid);
    void renderNavbar(std::vector<std::shared_ptr<Planet>> &planets);
};
