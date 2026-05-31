#include "UIManager.h"
#include "imgui/imgui.h"
#include "core/Constants.h"
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <limits>

void UIManager::render(Window& window, Camera& camera, float deltaTime, std::vector<CelestialBody>& bodies,
                       Grid& /*grid*/, PhysicsSystem& physics, GridSettings& gridSettings)
{
    int width, height;
    glfwGetFramebufferSize(window.getGLFWwindow(), &width, &height);
    glm::mat4 view       = camera.getViewMatrix();
    float aspect         = static_cast<float>(width) / static_cast<float>(std::max(height, 1));
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 20000.0f);

    actionbar_.renderTopBar(window, camera, physics, bodies, *this, gridSettings, deltaTime);
    renderPlanetPopup(window, camera, view, projection, bodies);
    renderBodyContextMenu(bodies, camera);

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
            lastSelectedIndex   = selectedPlanetIndex;
        }

        renderPlanetInfo(body);
    }

    if (showDiagnostics)
        renderDiagnostics(bodies);
    if (showBodyLabels)
        renderPersistentBodyLabels(window, camera, view, projection, bodies);

    actionbar_.renderBottomBar(window, *this, gridSettings);

    // Deferred removal — set inside renderPlanetInfo; processed here so we don't
    // mutate the bodies vector while the panel still holds a reference to it.
    if (pendingRemove && selectedPlanetIndex >= 0 && selectedPlanetIndex < static_cast<int>(bodies.size()))
    {
        const std::string removedName = bodies[selectedPlanetIndex].name;
        camera.shiftOrbitalIndexOnRemove(selectedPlanetIndex);
        bodies.erase(bodies.begin() + selectedPlanetIndex);
        selectedPlanetIndex = -1;
        lastSelectedIndex   = -1;
        hoveredIndex        = -1;
        diagBaselineSet     = false; // body count changed → invalidate baseline
        toasts_.info("Removed " + removedName);
    }
    pendingRemove = false;

    toasts_.update(deltaTime);
    toasts_.render();
}

void UIManager::renderPlanetPopup(Window& window, Camera& camera, const glm::mat4& view,
                                  const glm::mat4& projection, std::vector<CelestialBody>& bodies)
{
    double mouseX, mouseY;
    glfwGetCursorPos(window.getGLFWwindow(), &mouseX, &mouseY);
    int width, height;
    glfwGetFramebufferSize(window.getGLFWwindow(), &width, &height);
    glm::vec3 rayDir  = camera.getRayFromMouse(mouseX, mouseY, width, height, view, projection);
    glm::vec3 rayOrig = camera.getPosition();

    // Ray-vs-ecliptic-plane intersect (y = 0). Returns a sane fallback
    // (a point in front of the camera) when the ray is parallel or
    // pointing the wrong way.
    auto castToEclipticPlane = [&]() -> glm::vec3
    {
        constexpr float kFallback = 100.0f;
        if (std::abs(rayDir.y) < 1e-6f)
            return rayOrig + rayDir * kFallback;
        const float t = -rayOrig.y / rayDir.y;
        if (t < 0.0f)
            return rayOrig + rayDir * kFallback;
        return rayOrig + t * rayDir;
    };

    // Update an in-flight placement regardless of ImGui mouse-capture state —
    // the drag started outside the panels, so the user should be able to
    // release it anywhere (including over the Planet Info dock).
    if (placementActive_)
    {
        placementEndPosWU_ = castToEclipticPlane();
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            spawnPlacedBody(bodies);
            placementActive_ = false;
        }
    }

    if (ImGui::GetIO().WantCaptureMouse)
    {
        hoveredIndex = -1;
        return;
    }

    hoveredIndex   = -1;
    float minDist2 = std::numeric_limits<float>::max();
    for (size_t i = 0; i < bodies.size(); ++i)
    {
        if (bodies[i].intersectsRay(rayOrig, rayDir))
        {
            glm::vec3 d = bodies[i].renderPosition() - rayOrig;
            float d2    = glm::dot(d, d);
            if (d2 < minDist2)
            {
                minDist2     = d2;
                hoveredIndex = static_cast<int>(i);
            }
        }
    }
    if (hoveredIndex != -1)
    {
        glm::vec3 worldAbove =
            bodies[hoveredIndex].renderPosition() + glm::vec3(0.0f, bodies[hoveredIndex].radius, 0.0f);
        glm::vec2 screenPos = camera.worldToScreen(worldAbove, view, projection, width, height);
        if (screenPos.x > 0 && screenPos.y > 0 && screenPos.x < width && screenPos.y < height)
        {
            ImGui::SetNextWindowPos(ImVec2(screenPos.x, screenPos.y));
            ImGui::Begin("##planet_label", nullptr,
                         ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar |
                             ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::Text("%s", bodies[hoveredIndex].name.c_str());
            ImGui::End();
        }
    }

    // LMB press → either remember the hovered body (fire on release if it was
    // a true click, not a drag) or begin a drag-to-place on empty space.
    //
    // The fly-to is deferred to release because synthetic touch→mouse events
    // fire mousedown the instant a finger lands — without this guard, any
    // camera-rotation drag that starts on top of a planet would immediately
    // teleport the camera to that planet before the drag could play out.
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        if (hoveredIndex >= 0)
        {
            lmbPressedTargetIdx_ = hoveredIndex;
        }
        // On touch devices the single finger is reserved for camera rotation,
        // so only begin a drag-to-place when the user has explicitly enabled
        // placement mode. Desktop keeps the unconditional LMB-in-empty-space
        // gesture (camera there is a separate button).
        else if (!placementActive_ && (!touchDevice || placementModeActive))
        {
            placementStartPosWU_ = castToEclipticPlane();
            placementEndPosWU_   = placementStartPosWU_;
            placementActive_     = true;
        }
    }

    constexpr float kClickThresholdSq = 16.0f; // 4 px — shared between LMB and RMB.

    // LMB release on a body — only treat as a select+fly if the cursor barely
    // moved between press and release (genuine click, not the tail of a drag).
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) && lmbPressedTargetIdx_ != -1)
    {
        const ImVec2 drag     = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
        const float dragLenSq = drag.x * drag.x + drag.y * drag.y;
        if (dragLenSq < kClickThresholdSq && lmbPressedTargetIdx_ < static_cast<int>(bodies.size()))
        {
            selectedPlanetIndex  = lmbPressedTargetIdx_;
            auto& selected       = bodies[lmbPressedTargetIdx_];
            const float distance = selected.focusDistance();
            if (camera.getMode() == CameraMode::ORBITAL)
            {
                camera.flyToOrbital(lmbPressedTargetIdx_, selected.renderPosition(), distance);
            }
            else
            {
                camera.startSmoothMove(selected.renderPosition(), distance);
            }
        }
        lmbPressedTargetIdx_ = -1;
        ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
    }

    // Right-click on a body opens the context menu — but right-drag is also
    // the mouse-look gesture, so we only treat it as a click if the cursor
    // barely moved between press and release.
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    {
        rmbPressedTargetIdx_ = hoveredIndex;
    }
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Right))
    {
        const ImVec2 drag     = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right);
        const float dragLenSq = drag.x * drag.x + drag.y * drag.y;
        if (rmbPressedTargetIdx_ != -1 && dragLenSq < kClickThresholdSq)
        {
            contextMenuTargetIdx_ = rmbPressedTargetIdx_;
            openContextMenuPopup_ = true;
        }
        rmbPressedTargetIdx_ = -1;
        ImGui::ResetMouseDragDelta(ImGuiMouseButton_Right);
    }
}

void UIManager::spawnPlacedBody(std::vector<CelestialBody>& bodies)
{
    const glm::dvec3 startMeters = glm::dvec3(placementStartPosWU_) * METERS_PER_WU;
    const glm::dvec3 dragVecWU   = glm::dvec3(placementEndPosWU_ - placementStartPosWU_);

    // Velocity scale chosen so a ~100 WU drag at 1 AU yields a near-circular
    // orbital velocity for an Earth-mass body. Below that → spiral inward.
    constexpr double kVelocityPerWU = 300.0;
    const glm::dvec3 velocity       = dragVecWU * kVelocityPerWU;

    CelestialBody body;
    body.pos_m        = startMeters;
    body.vel_m        = velocity;
    body.mass_kg      = 5.972e24; // Earth-mass default
    body.name         = "Custom " + std::to_string(bodies.size() + 1);
    body.color        = glm::vec3(0.40f, 0.70f, 1.00f);
    body.density      = 3000.0f;
    body.emissive     = 0.0f;
    body.displayScale = 1.0f;
    body.recalculateGeometry();

    const std::string addedName = body.name;
    bodies.push_back(std::move(body));
    setSelected(static_cast<int>(bodies.size()) - 1);

    const double speedKmS = glm::length(velocity) / 1000.0;
    char buf[96];
    std::snprintf(buf, sizeof(buf), "Placed %s — initial v = %.1f km/s", addedName.c_str(), speedKmS);
    toasts_.success(buf);
}

void UIManager::renderPlanetInfo(CelestialBody& body)
{
    // Forced dock flush against the top actionbar — Planet Info now reads
    // as a "drops down from the top bar" panel rather than a floating
    // window the user can shove anywhere. ImGuiCond_Always so the lock is
    // re-applied every frame (so the window can't be dragged or resized);
    // the `##v3` suffix invalidates any stale imgui.ini layout from the
    // previous floating version.
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    constexpr float kPanelWidth   = 340.0f;
    ImGui::SetNextWindowPos(
        ImVec2(viewport->WorkPos.x + viewport->WorkSize.x - kPanelWidth, viewport->WorkPos.y),
        ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(kPanelWidth, 0.0f), ImGuiCond_Always);

    constexpr ImGuiWindowFlags kFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                                        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;

    ImGui::Begin("Planet Info##v3", nullptr, kFlags);

    // Type chip + current name at the top — gives the panel an identity
    // header before the editable buffer below.
    const BodyType bodyType = body.classify();
    const glm::vec3 chipRgb = bodyTypeColor(bodyType);
    const ImVec4 chipColor(chipRgb.r, chipRgb.g, chipRgb.b, 1.0f);
    ImGui::TextColored(chipColor, "[%s]", bodyTypeLabel(bodyType));
    ImGui::SameLine();
    ImGui::TextUnformatted(body.name.c_str());
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::InputText("Name", editBuffer.name, sizeof(editBuffer.name));
    ImGui::InputFloat("Mass (kg)", &editBuffer.mass, 0.0f, 0.0f, "%.3e");
    ImGui::InputFloat("Density (kg/m^3)", &editBuffer.density);
    ImGui::InputFloat3("Position (WU)", &editBuffer.position[0]);
    ImGui::InputFloat3("Velocity (km/s)", &editBuffer.velocity[0]);

    if (ImGui::Button("Apply Changes"))
    {
        body.name    = editBuffer.name;
        body.mass_kg = static_cast<double>(editBuffer.mass);
        body.density = editBuffer.density;
        body.pos_m   = glm::dvec3(editBuffer.position) * METERS_PER_WU;
        body.vel_m   = glm::dvec3(editBuffer.velocity) * 1000.0;
        body.recalculateGeometry();
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset"))
    {
        std::snprintf(editBuffer.name, sizeof(editBuffer.name), "%s", body.name.c_str());
        editBuffer.mass     = static_cast<float>(body.mass_kg);
        editBuffer.density  = body.density;
        editBuffer.position = glm::vec3(body.pos_m / METERS_PER_WU);
        editBuffer.velocity = glm::vec3(body.vel_m / 1000.0);
    }
    ImGui::SameLine();
    if (ImGui::Button("Remove"))
    {
        pendingRemove = true;
    }

    // Per-body visualisations — only make sense in the context of a
    // selected body, so they live here rather than in the bottom bar
    // (which now hosts only system-wide toggles).
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextDisabled("Visualisations");
    ImGui::Checkbox("Path prediction", &showPathPrediction);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Project this body's future orbit as a dashed line");
    ImGui::Checkbox("Lagrange points", &showLagrange);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Mark L1\xe2\x80\x93L5 between the most massive body and this one");

    ImGui::End();
}

bool UIManager::isRightMousePressed(GLFWwindow* window)
{
    return !ImGui::GetIO().WantCaptureMouse &&
           glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
}

void UIManager::renderDiagnostics(const std::vector<CelestialBody>& bodies)
{
    // Dock at the left edge below the top actionbar on first use; the `##v2`
    // suffix invalidates any pre-actionbar position saved in imgui.ini.
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    constexpr float kPanelWidth   = 360.0f;
    constexpr float kMargin       = 16.0f;
    ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + kMargin, viewport->WorkPos.y + kMargin),
                            ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(kPanelWidth, 0.0f), ImGuiCond_FirstUseEver);

    ImGui::Begin("Diagnostics##v2", &showDiagnostics);

    double KE = 0.0, PE = 0.0;
    glm::dvec3 p_total(0.0);
    glm::dvec3 L_total(0.0);
    for (size_t i = 0; i < bodies.size(); ++i)
    {
        const auto& bi = bodies[i];
        p_total += bi.vel_m * bi.mass_kg;
        L_total += bi.mass_kg * glm::cross(bi.pos_m, bi.vel_m);
        KE += 0.5 * bi.mass_kg * glm::dot(bi.vel_m, bi.vel_m);
        for (size_t j = i + 1; j < bodies.size(); ++j)
        {
            const auto& bj     = bodies[j];
            const glm::dvec3 r = bj.pos_m - bi.pos_m;
            const double dist  = std::sqrt(glm::dot(r, r) + PhysicsSystem::SOFTEN);
            PE -= PhysicsSystem::G * bi.mass_kg * bj.mass_kg / dist;
        }
    }
    const double E    = KE + PE;
    const double pmag = glm::length(p_total);
    const double Lmag = glm::length(L_total);

    if (!diagBaselineSet)
    {
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

    if (ImGui::SmallButton("Reset baseline"))
    {
        diagBaselineSet = false;
    }

    ImGui::End();
}

void UIManager::renderPersistentBodyLabels(Window& window, Camera& camera, const glm::mat4& view,
                                           const glm::mat4& projection,
                                           const std::vector<CelestialBody>& bodies)
{
    int width, height;
    glfwGetFramebufferSize(window.getGLFWwindow(), &width, &height);

    constexpr ImGuiWindowFlags kFlags =
        ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav;

    for (size_t i = 0; i < bodies.size(); ++i)
    {
        const auto& body           = bodies[i];
        const float displayR       = body.radius * body.displayScale;
        const glm::vec3 worldAbove = body.renderPosition() + glm::vec3(0.0f, displayR * 1.8f + 0.5f, 0.0f);

        const glm::vec2 screenPos = camera.worldToScreen(worldAbove, view, projection, width, height);
        if (screenPos.x < 0.0f || screenPos.y < 0.0f || screenPos.x > static_cast<float>(width) ||
            screenPos.y > static_cast<float>(height))
            continue;

        const glm::vec3 chipRgb = bodyTypeColor(body.classify());
        const ImVec4 color(chipRgb.r, chipRgb.g, chipRgb.b, 0.85f);

        // Anchor centre-bottom so the label floats just above the body and
        // stays centred horizontally as the camera pans.
        ImGui::SetNextWindowPos(ImVec2(screenPos.x, screenPos.y), ImGuiCond_Always, ImVec2(0.5f, 1.0f));
        ImGui::PushID(static_cast<int>(i));
        ImGui::Begin("##bodylabel", nullptr, kFlags);
        ImGui::TextColored(color, "%s", body.name.c_str());
        ImGui::End();
        ImGui::PopID();
    }
}

void UIManager::renderBodyContextMenu(std::vector<CelestialBody>& bodies, Camera& camera)
{
    constexpr const char* kPopupId = "##bodyContextMenu";

    if (openContextMenuPopup_)
    {
        ImGui::OpenPopup(kPopupId);
        openContextMenuPopup_ = false;
    }

    if (!ImGui::BeginPopup(kPopupId))
        return;

    const bool validTarget =
        (contextMenuTargetIdx_ >= 0 && contextMenuTargetIdx_ < static_cast<int>(bodies.size()));
    if (!validTarget)
    {
        ImGui::EndPopup();
        return;
    }

    CelestialBody& target = bodies[contextMenuTargetIdx_];
    ImGui::TextDisabled("%s", target.name.c_str());
    ImGui::Separator();

    if (ImGui::MenuItem("Focus"))
    {
        camera.flyToOrbital(contextMenuTargetIdx_, target.renderPosition(), target.focusDistance());
    }
    if (ImGui::MenuItem("Edit"))
    {
        setSelected(contextMenuTargetIdx_);
    }
    if (ImGui::MenuItem("Make Star", nullptr, false, target.emissive < 0.5f))
    {
        target.emissive     = 1.0f;
        target.color        = glm::vec3(1.0f, 0.85f, 0.45f);
        target.displayScale = 0.4f;
        toasts_.success(target.name + " is now a star");
    }
    if (ImGui::MenuItem("Duplicate"))
    {
        CelestialBody copy;
        const glm::dvec3 velocityDirection =
            glm::length(target.vel_m) > 1.0 ? glm::normalize(target.vel_m) : glm::dvec3(1.0, 0.0, 0.0);
        const glm::dvec3 perpendicular =
            glm::normalize(glm::cross(velocityDirection, glm::dvec3(0.0, 1.0, 0.0)));
        constexpr double kDuplicateOffsetWU = 20.0;

        copy.pos_m        = target.pos_m + perpendicular * kDuplicateOffsetWU * METERS_PER_WU;
        copy.vel_m        = target.vel_m;
        copy.mass_kg      = target.mass_kg;
        copy.name         = target.name + " (copy)";
        copy.color        = target.color;
        copy.density      = target.density;
        copy.emissive     = target.emissive;
        copy.displayScale = target.displayScale;
        copy.hasRings     = target.hasRings;
        copy.recalculateGeometry();

        const std::string newName = copy.name;
        bodies.push_back(std::move(copy));
        setSelected(static_cast<int>(bodies.size()) - 1);
        toasts_.success("Duplicated as " + newName);
    }

    ImGui::Separator();
    if (ImGui::MenuItem("Remove"))
    {
        selectedPlanetIndex = contextMenuTargetIdx_;
        pendingRemove       = true;
    }

    ImGui::EndPopup();
}
