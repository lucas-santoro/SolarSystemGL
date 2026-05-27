#include "AddPlanetModal.h"

#include "ui/UIManager.h" // pulls glad before any GLFW header
#include "ui/UI.h"
#include "core/Constants.h"
#include "objects/CelestialBody.h"
#include "physics/PhysicsSystem.h"

#include <imgui.h>

#include <cmath>
#include <string>

namespace
{
constexpr double kSunMassKg           = 1.989e30;
constexpr double kNewPlanetOrbitAU    = 1.5;
constexpr double kBodyToBodyYawOffset = 0.7;
constexpr float kNewPlanetDensity     = 3000.0f;
} // namespace

void AddPlanetModal::render(std::vector<CelestialBody>& bodies, UIManager& uiManager)
{
    if (pendingOpen_)
    {
        ImGui::OpenPopup(kPopupId);
        pendingOpen_ = false;
    }

    const ImVec2 viewportCenter = ImGui::GetMainViewport()->GetCenter();
    constexpr ImVec2 kButtonSize(120.0f, 0.0f);

    ImGui::SetNextWindowPos(viewportCenter, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ui::modalSize(360.0f, 0.0f), ImGuiCond_Appearing);

    if (!ImGui::BeginPopupModal(kPopupId, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        return;
    }

    ImGui::InputText("Name", newPlanetName_, sizeof(newPlanetName_));
    ImGui::ColorEdit3("Color", &newPlanetColor_[0]);
    ImGui::InputFloat("Mass (kg)", &newPlanetMass_, 0.0f, 0.0f, "%.3e");

    ImGui::Spacing();
    if (ImGui::Button("Add", kButtonSize))
    {
        const double semiMajorAxisMeters = kNewPlanetOrbitAU * AU;
        const double yawAngle            = static_cast<double>(bodies.size()) * kBodyToBodyYawOffset;
        const double orbitalSpeed        = std::sqrt(PhysicsSystem::G * kSunMassKg / semiMajorAxisMeters);

        CelestialBody body;
        body.pos_m   = glm::dvec3(semiMajorAxisMeters * std::cos(yawAngle), 0.0,
                                  semiMajorAxisMeters * std::sin(yawAngle));
        body.vel_m   = glm::dvec3(-orbitalSpeed * std::sin(yawAngle), 0.0, orbitalSpeed * std::cos(yawAngle));
        body.mass_kg = static_cast<double>(newPlanetMass_);
        body.name    = newPlanetName_;
        body.color   = newPlanetColor_;
        body.density = kNewPlanetDensity;
        body.recalculateGeometry();

        const std::string addedName = body.name;
        bodies.push_back(std::move(body));
        uiManager.setSelected(static_cast<int>(bodies.size()) - 1);
        uiManager.toasts().success("Added " + addedName);

        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", kButtonSize))
    {
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}
