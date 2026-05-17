#pragma once

#include <glm/glm.hpp>

#include <vector>

class UIManager;
struct CelestialBody;

/**
 * @file AddPlanetModal.h
 * @brief Centered popup that builds a new CelestialBody from user inputs.
 */

/**
 * @brief "Add Planet" popup — name, color, mass; appends to the bodies vector.
 *
 * The newly added body is placed at a fixed semi-major axis of 1.5 AU with a
 * circular orbital velocity tuned to the Sun's mass. The yaw angle is
 * derived from the current body count so multiple new planets don't pile up.
 */
class AddPlanetModal
{
public:
    static constexpr const char* kPopupId = "Add Planet";

    /// Draws the popup and applies the user's input on the Add button.
    void render(std::vector<CelestialBody>& bodies, UIManager& uiManager);

    /// Mark the popup as pending-open. The next #render call issues OpenPopup.
    void requestOpen() { pendingOpen_ = true; }

private:
    bool      pendingOpen_           = false;
    char      newPlanetName_[128]    = "New Planet";
    glm::vec3 newPlanetColor_{ 0.8f, 0.8f, 0.9f };
    float     newPlanetMass_         = 1.0e24f;
};
