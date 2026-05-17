#include "core/Application.h"

#include "core/Constants.h"
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"

#include <algorithm>
#include <cmath>

namespace
{
// Trail sampling tuning.
constexpr size_t kTrailHistoryCapacity = 512;
constexpr float  kMinimumTrailStepWU   = 0.2f;
constexpr float  kTrailTeleportJumpWU  = 100.0f;

// Procedural ring mesh tuning.
constexpr int    kRingSegmentCount   = 64;
constexpr float  kRingInnerRadius    = 1.0f;
constexpr float  kRingOuterRadius    = 1.5f;
constexpr float  kRingScaleMultiplier = 1.5f;

// Star halo billboarding.
constexpr float  kHaloSizeMultiplier = 3.0f;

// Camera and projection defaults.
const     glm::vec3 kInitialCameraPosition{ 0.0f, 0.0f, 300.0f };
constexpr float  kProjectionFovDegrees    = 45.0f;
constexpr float  kProjectionNearPlane     = 0.01f;
constexpr float  kProjectionFarPlane      = 10000.0f;

// Scratch-vector reserve (prevents reallocations during typical "Add Planet" sessions).
constexpr size_t kBodiesReserveCapacity = 16;

// Visual ring color.
const     glm::vec3 kSaturnRingColor{ 0.90f, 0.85f, 0.70f };
} // namespace

//----------------------------------------------------------------------------
// Static callbacks
//----------------------------------------------------------------------------

Application* Application::fromWindow(GLFWwindow* window)
{
    return static_cast<Application*>(glfwGetWindowUserPointer(window));
}

void Application::mouseCallback(GLFWwindow* window, double xpos, double ypos)
{
    Application* app = fromWindow(window);
    if (app == nullptr) return;

    if (app->firstMouseEvent_)
    {
        app->lastMouseX_      = static_cast<float>(xpos);
        app->lastMouseY_      = static_cast<float>(ypos);
        app->firstMouseEvent_ = false;
    }

    const float xOffset = static_cast<float>(xpos) - app->lastMouseX_;
    const float yOffset = app->lastMouseY_ - static_cast<float>(ypos);
    app->lastMouseX_    = static_cast<float>(xpos);
    app->lastMouseY_    = static_cast<float>(ypos);

    if (app->uiManager_.isRightMousePressed(window))
    {
        app->camera_.processMouseMovement(xOffset, yOffset);
    }
}

void Application::scrollCallback(GLFWwindow* window, double /*xoffset*/, double yoffset)
{
    Application* app = fromWindow(window);
    if (app == nullptr || ImGui::GetIO().WantCaptureMouse) return;

    app->camera_.processKeyboard(yoffset > 0 ? GLFW_KEY_W : GLFW_KEY_S, 0.1f);
}

void Application::windowFocusCallback(GLFWwindow* window, int focused)
{
    Application* app = fromWindow(window);
    if (app == nullptr) return;

    if (focused == GLFW_FALSE)
    {
        // Only auto-pause if the simulation is actively running and isn't
        // already paused (so manual pauses don't get clobbered on resume).
        if (app->appState_ == AppState::Running && !app->physics_.paused)
        {
            app->physics_.paused = true;
            app->wasAutoPaused_  = true;
        }
    }
    else if (app->wasAutoPaused_)
    {
        app->physics_.paused = false;
        app->wasAutoPaused_  = false;
    }
}

void Application::windowCloseCallback(GLFWwindow* window)
{
    Application* app = fromWindow(window);
    if (app == nullptr) return;

    if (app->appState_ == AppState::Running)
    {
        // Defer the close until the user confirms.
        glfwSetWindowShouldClose(window, GLFW_FALSE);
        app->openQuitPopup_ = true;
    }
}

//----------------------------------------------------------------------------
// Construction / destruction
//----------------------------------------------------------------------------

Application::Application(int windowWidth, int windowHeight, const char* title)
    : window_(windowWidth, windowHeight, title)
    , camera_(kInitialCameraPosition)
    , bodyShader_ ("shaders/VertexShader.glsl",      "shaders/FragmentShader.glsl")
    , gridShader_ ("shaders/GridVertexShader.glsl",  "shaders/GridFragmentShader.glsl")
    , trailShader_("shaders/TrailVertexShader.glsl", "shaders/TrailFragmentShader.glsl")
    , haloShader_ ("shaders/HaloVertexShader.glsl",  "shaders/HaloFragmentShader.glsl")
    , skyShader_  ("shaders/SkyVertexShader.glsl",   "shaders/SkyFragmentShader.glsl")
    , ringShader_ ("shaders/RingVertexShader.glsl",  "shaders/RingFragmentShader.glsl")
    , physics_()
    , uiManager_()
    , grid_(10000.0f, 200, 0.0f)
{
    bodies_.reserve(kBodiesReserveCapacity);

    GLFWwindow* glfwWindow = window_.getGLFWwindow();
    glfwSetWindowUserPointer(glfwWindow, this);
    glfwSetFramebufferSizeCallback(glfwWindow,
        [](GLFWwindow*, int w, int h) { glViewport(0, 0, w, h); });
    glfwSetCursorPosCallback(glfwWindow,   mouseCallback);
    glfwSetScrollCallback(glfwWindow,      scrollCallback);
    glfwSetWindowFocusCallback(glfwWindow, windowFocusCallback);
    glfwSetWindowCloseCallback(glfwWindow, windowCloseCallback);
    glfwSetInputMode(glfwWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    createAuxiliaryBuffers();
    // Bodies are deferred to startSimulation() — the menu screen runs first
    // and only loads the default solar system when the user clicks Start.

    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(glfwWindow, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    glfwSwapInterval(uiManager_.vsync ? 1 : 0);
}

Application::~Application()
{
    destroyAuxiliaryBuffers();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

//----------------------------------------------------------------------------
// Public API
//----------------------------------------------------------------------------

void Application::run()
{
    while (!glfwWindowShouldClose(window_.getGLFWwindow()))
    {
        tick();
    }
}

//----------------------------------------------------------------------------
// Frame loop
//----------------------------------------------------------------------------

void Application::tick()
{
    const float currentFrameTime = static_cast<float>(glfwGetTime());
    const float deltaTime        = currentFrameTime - previousFrameTime_;
    previousFrameTime_ = currentFrameTime;

    if (appState_ == AppState::Menu)
    {
        renderMenuFrame(deltaTime);
        return;
    }

    refreshOrbitalCameraTarget();
    camera_.update(deltaTime);

    GLFWwindow* glfwWindow = window_.getGLFWwindow();
    if (glfwGetKey(glfwWindow, GLFW_KEY_W) == GLFW_PRESS) camera_.processKeyboard(GLFW_KEY_W, deltaTime);
    if (glfwGetKey(glfwWindow, GLFW_KEY_S) == GLFW_PRESS) camera_.processKeyboard(GLFW_KEY_S, deltaTime);
    if (glfwGetKey(glfwWindow, GLFW_KEY_A) == GLFW_PRESS) camera_.processKeyboard(GLFW_KEY_A, deltaTime);
    if (glfwGetKey(glfwWindow, GLFW_KEY_D) == GLFW_PRESS) camera_.processKeyboard(GLFW_KEY_D, deltaTime);

    handleHotkeys();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    int framebufferWidth = 0, framebufferHeight = 0;
    glfwGetFramebufferSize(glfwWindow, &framebufferWidth, &framebufferHeight);
    const float aspectRatio = static_cast<float>(framebufferWidth)
                            / static_cast<float>(std::max(framebufferHeight, 1));
    const glm::mat4 view       = camera_.getViewMatrix();
    const glm::mat4 projection = glm::perspective(glm::radians(kProjectionFovDegrees),
                                                  aspectRatio,
                                                  kProjectionNearPlane,
                                                  kProjectionFarPlane);

    physics_.update(bodies_, static_cast<double>(deltaTime));
    sampleOrbitTrails();

    renderSky(view, projection);
    renderBodies(view, projection);
    renderGrid(view, projection);
    renderRings(view, projection);

    if (uiManager_.showTrails) renderTrails(view, projection);
    if (uiManager_.showBloom)  renderHalos(view, projection);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    uiManager_.render(window_, camera_, deltaTime, bodies_, grid_, physics_);

    if (uiManager_.menuRequested)
    {
        openReturnToMenuPopup_ = true;
        uiManager_.menuRequested = false;
    }
    if (uiManager_.settingsRequested)
    {
        uiManager_.toasts().info("Settings modal coming soon");
        uiManager_.settingsRequested = false;
    }
    if (uiManager_.saveRequested)
    {
        uiManager_.toasts().info("Save modal coming soon — use Solar System panel for now");
        uiManager_.saveRequested = false;
    }
    if (uiManager_.loadRequested)
    {
        uiManager_.toasts().info("Load modal coming soon — use Solar System panel for now");
        uiManager_.loadRequested = false;
    }

    renderConfirmModals();

    if (uiManager_.vsyncDirty)
    {
        glfwSwapInterval(uiManager_.vsync ? 1 : 0);
        uiManager_.vsyncDirty = false;
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(glfwWindow);
    glfwPollEvents();
}

//----------------------------------------------------------------------------
// Menu state
//----------------------------------------------------------------------------

void Application::renderMenuFrame(float deltaTime)
{
    menuTime_ += deltaTime;

    GLFWwindow* glfwWindow = window_.getGLFWwindow();

    int framebufferWidth = 0, framebufferHeight = 0;
    glfwGetFramebufferSize(glfwWindow, &framebufferWidth, &framebufferHeight);
    const float aspectRatio = static_cast<float>(framebufferWidth)
                            / static_cast<float>(std::max(framebufferHeight, 1));

    // Slow camera orbit around the origin for a cinematic skybox.
    constexpr float kMenuOrbitRadius      = 300.0f;
    constexpr float kMenuOrbitHeight      = 60.0f;
    constexpr float kMenuOrbitAngularRate = 0.05f;
    const float angle = menuTime_ * kMenuOrbitAngularRate;
    const glm::vec3 menuCameraPosition(kMenuOrbitRadius * std::cos(angle),
                                       kMenuOrbitHeight,
                                       kMenuOrbitRadius * std::sin(angle));

    const glm::mat4 view       = glm::lookAt(menuCameraPosition,
                                             glm::vec3(0.0f),
                                             glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::mat4 projection = glm::perspective(glm::radians(kProjectionFovDegrees),
                                                  aspectRatio,
                                                  kProjectionNearPlane,
                                                  kProjectionFarPlane);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    renderSky(view, projection);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    const StartMenuAction action = startMenu_.render(framebufferWidth, framebufferHeight);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(glfwWindow);
    glfwPollEvents();

    switch (action)
    {
        case StartMenuAction::Start:    startSimulation(); break;
        case StartMenuAction::Quit:     glfwSetWindowShouldClose(glfwWindow, true); break;
        case StartMenuAction::Load:     /* Wired in Task 12. */ break;
        case StartMenuAction::Settings: /* Wired in Task 11. */ break;
        case StartMenuAction::None:     break;
    }
}

void Application::startSimulation()
{
    bodies_.clear();
    loadDefaultBodies();
    shiftToBarycenterFrame();
    camera_.reset();
    appState_ = AppState::Running;
    previousFrameTime_ = static_cast<float>(glfwGetTime());
}

void Application::returnToMenu()
{
    bodies_.clear();
    uiManager_.clearSelection();
    camera_.reset();
    appState_ = AppState::Menu;
    menuTime_ = 0.0f;
    previousFrameTime_ = static_cast<float>(glfwGetTime());
}

void Application::renderConfirmModals()
{
    constexpr const char* kReturnPopupId = "Return to Menu?";
    constexpr const char* kQuitPopupId   = "Quit Application?";

    if (openReturnToMenuPopup_)
    {
        ImGui::OpenPopup(kReturnPopupId);
        openReturnToMenuPopup_ = false;
    }
    if (openQuitPopup_)
    {
        ImGui::OpenPopup(kQuitPopupId);
        openQuitPopup_ = false;
    }

    const ImVec2 viewportCenter = ImGui::GetMainViewport()->GetCenter();
    constexpr ImVec2 kButtonSize(120.0f, 0.0f);

    ImGui::SetNextWindowPos(viewportCenter, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal(kReturnPopupId, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextUnformatted("Discard the current simulation and return to the main menu?");
        ImGui::Spacing();
        if (ImGui::Button("Yes", kButtonSize))
        {
            returnToMenu();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("No", kButtonSize))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::SetNextWindowPos(viewportCenter, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal(kQuitPopupId, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextUnformatted("Quit SolarSystemGL?");
        ImGui::Spacing();
        if (ImGui::Button("Quit", kButtonSize))
        {
            glfwSetWindowShouldClose(window_.getGLFWwindow(), GLFW_TRUE);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", kButtonSize))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

//----------------------------------------------------------------------------
// Initialization helpers
//----------------------------------------------------------------------------

void Application::loadDefaultBodies()
{
    struct DefaultBody
    {
        const char* name;
        double      semiMajorAxisAU;
        double      orbitalVelocityMS;
        double      massKg;
        float       densityKgPerM3;
        glm::vec3   color;
        float       emissive;
        float       displayScale;
        bool        hasRings;
    };

    const DefaultBody defaults[] = {
        { "Sun",      0.000,     0.0, 1.989e30, 1408.0f, glm::vec3(1.00f, 0.90f, 0.30f), 1.0f, 0.4f, false },
        { "Mercury",  0.387, 47900.0, 3.3011e23, 5427.0f, glm::vec3(0.50f, 0.50f, 0.50f), 0.0f, 1.0f, false },
        { "Venus",    0.723, 35000.0, 4.8675e24, 5243.0f, glm::vec3(0.95f, 0.85f, 0.55f), 0.0f, 1.0f, false },
        { "Earth",    1.000, 29780.0, 5.9720e24, 5514.0f, glm::vec3(0.20f, 0.40f, 1.00f), 0.0f, 1.0f, false },
        { "Mars",     1.524, 24100.0, 6.4171e23, 3933.0f, glm::vec3(0.80f, 0.30f, 0.10f), 0.0f, 1.0f, false },
        { "Jupiter",  5.203, 13070.0, 1.8980e27, 1326.0f, glm::vec3(0.90f, 0.70f, 0.40f), 0.0f, 1.0f, false },
        { "Saturn",   9.537,  9680.0, 5.6834e26,  687.0f, glm::vec3(0.95f, 0.85f, 0.50f), 0.0f, 1.0f, true  },
        { "Uranus",  19.191,  6800.0, 8.6810e25, 1271.0f, glm::vec3(0.60f, 0.85f, 0.90f), 0.0f, 1.0f, false },
        { "Neptune", 30.070,  5430.0, 1.0240e26, 1638.0f, glm::vec3(0.30f, 0.40f, 0.85f), 0.0f, 1.0f, false },
    };

    for (const auto& spec : defaults)
    {
        CelestialBody body;
        body.pos_m        = glm::dvec3(spec.semiMajorAxisAU * AU, 0.0, 0.0);
        body.vel_m        = glm::dvec3(0.0, 0.0, spec.orbitalVelocityMS);
        body.mass_kg      = spec.massKg;
        body.name         = spec.name;
        body.color        = spec.color;
        body.density      = spec.densityKgPerM3;
        body.emissive     = spec.emissive;
        body.displayScale = spec.displayScale;
        body.hasRings     = spec.hasRings;
        body.recalculateGeometry();
        bodies_.push_back(std::move(body));
    }
}

void Application::shiftToBarycenterFrame()
{
    glm::dvec3 totalMomentum(0.0);
    double     totalMass = 0.0;
    for (const auto& body : bodies_)
    {
        totalMomentum += body.vel_m * body.mass_kg;
        totalMass     += body.mass_kg;
    }
    if (totalMass <= 0.0) return;

    const glm::dvec3 centerOfMassVelocity = totalMomentum / totalMass;
    for (auto& body : bodies_)
    {
        body.vel_m -= centerOfMassVelocity;
    }
}

void Application::createAuxiliaryBuffers()
{
    // Dynamic VBO for trail line strips (re-uploaded per body per frame).
    glGenVertexArrays(1, &trailVAO_);
    glGenBuffers(1, &trailVBO_);
    glBindVertexArray(trailVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, trailVBO_);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Static fullscreen unit quad: used by both the halo billboard and the sky.
    const float fullscreenQuadVertices[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
        -1.0f,  1.0f,
         1.0f,  1.0f,
    };
    glGenVertexArrays(1, &haloVAO_);
    glGenBuffers(1, &haloVBO_);
    glBindVertexArray(haloVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, haloVBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(fullscreenQuadVertices), fullscreenQuadVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Ring mesh: triangle strip on the XZ plane, inner radius 1.0, outer 1.5.
    std::vector<glm::vec3> ringVertices;
    ringVertices.reserve(static_cast<size_t>(2 * (kRingSegmentCount + 1)));
    for (int segment = 0; segment <= kRingSegmentCount; ++segment)
    {
        const float angle = 2.0f * 3.14159265358979f
                          * static_cast<float>(segment)
                          / static_cast<float>(kRingSegmentCount);
        const float cosA  = std::cos(angle);
        const float sinA  = std::sin(angle);
        ringVertices.emplace_back(cosA * kRingInnerRadius, 0.0f, sinA * kRingInnerRadius);
        ringVertices.emplace_back(cosA * kRingOuterRadius, 0.0f, sinA * kRingOuterRadius);
    }
    ringVertexCount_ = static_cast<GLsizei>(ringVertices.size());

    glGenVertexArrays(1, &ringVAO_);
    glGenBuffers(1, &ringVBO_);
    glBindVertexArray(ringVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, ringVBO_);
    glBufferData(GL_ARRAY_BUFFER,
                 ringVertices.size() * sizeof(glm::vec3),
                 ringVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Application::destroyAuxiliaryBuffers()
{
    if (trailVAO_) glDeleteVertexArrays(1, &trailVAO_);
    if (trailVBO_) glDeleteBuffers(1, &trailVBO_);
    if (haloVAO_)  glDeleteVertexArrays(1, &haloVAO_);
    if (haloVBO_)  glDeleteBuffers(1, &haloVBO_);
    if (ringVAO_)  glDeleteVertexArrays(1, &ringVAO_);
    if (ringVBO_)  glDeleteBuffers(1, &ringVBO_);
    trailVAO_ = trailVBO_ = haloVAO_ = haloVBO_ = ringVAO_ = ringVBO_ = 0;
}

//----------------------------------------------------------------------------
// Per-frame helpers
//----------------------------------------------------------------------------

void Application::refreshOrbitalCameraTarget()
{
    const int targetIndex = camera_.getOrbitalTargetIndex();
    if (targetIndex < 0 || targetIndex >= static_cast<int>(bodies_.size())) return;
    camera_.setOrbitalTargetPos(bodies_[targetIndex].renderPosition());
}

void Application::handleHotkeys()
{
    GLFWwindow* glfwWindow = window_.getGLFWwindow();
    const bool keyboardCaptured = ImGui::GetIO().WantCaptureKeyboard;

    const bool spaceDown = glfwGetKey(glfwWindow, GLFW_KEY_SPACE)  == GLFW_PRESS;
    const bool rDown     = glfwGetKey(glfwWindow, GLFW_KEY_R)      == GLFW_PRESS;
    const bool fDown     = glfwGetKey(glfwWindow, GLFW_KEY_F)      == GLFW_PRESS;
    const bool escDown   = glfwGetKey(glfwWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    const bool oneDown   = glfwGetKey(glfwWindow, GLFW_KEY_1)      == GLFW_PRESS;
    const bool twoDown   = glfwGetKey(glfwWindow, GLFW_KEY_2)      == GLFW_PRESS;

    if (!keyboardCaptured)
    {
        if (spaceDown && !wasSpacePressed_) physics_.paused = !physics_.paused;
        if (rDown     && !wasRPressed_)     camera_.reset();

        if (fDown && !wasFPressed_)
        {
            const int selected = uiManager_.getSelectedPlanetIndex();
            if (selected >= 0 && selected < static_cast<int>(bodies_.size()))
            {
                camera_.startSmoothMove(bodies_[selected].renderPosition(),
                                        bodies_[selected].focusDistance());
            }
        }

        if (escDown && !wasEscPressed_)
        {
            // Two-stage Esc: close info panel first, only prompt to leave the
            // simulation if nothing is selected.
            if (uiManager_.getSelectedPlanetIndex() >= 0)
            {
                uiManager_.clearSelection();
            }
            else
            {
                openReturnToMenuPopup_ = true;
            }
        }

        if (oneDown && !wasOnePressed_) camera_.setMode(CameraMode::FREE);
        if (twoDown && !wasTwoPressed_)
        {
            const int selected = uiManager_.getSelectedPlanetIndex();
            if (selected >= 0 && selected < static_cast<int>(bodies_.size()))
            {
                camera_.flyToOrbital(selected,
                                     bodies_[selected].renderPosition(),
                                     bodies_[selected].focusDistance());
            }
        }
    }

    wasSpacePressed_ = spaceDown;
    wasRPressed_     = rDown;
    wasFPressed_     = fDown;
    wasEscPressed_   = escDown;
    wasOnePressed_   = oneDown;
    wasTwoPressed_   = twoDown;
}

void Application::sampleOrbitTrails()
{
    for (auto& body : bodies_)
    {
        const glm::vec3 currentPosition = body.renderPosition();

        if (!body.trailPoints.empty())
        {
            const float jump = glm::distance(body.trailPoints.back(), currentPosition);
            if (jump > kTrailTeleportJumpWU) body.trailPoints.clear();
        }

        const bool firstSample = body.trailPoints.empty();
        const bool movedEnough = !firstSample
            && glm::distance(body.trailPoints.back(), currentPosition) > kMinimumTrailStepWU;

        if (firstSample || movedEnough)
        {
            body.trailPoints.push_back(currentPosition);
            if (body.trailPoints.size() > kTrailHistoryCapacity)
            {
                body.trailPoints.pop_front();
            }
        }
    }
}

//----------------------------------------------------------------------------
// Render passes
//----------------------------------------------------------------------------

void Application::renderSky(const glm::mat4& view, const glm::mat4& projection)
{
    skyShader_.use();
    skyShader_.setMat4("invView", glm::inverse(view));
    skyShader_.setMat4("invProj", glm::inverse(projection));
    glDisable(GL_DEPTH_TEST);
    glBindVertexArray(haloVAO_);   // re-use the fullscreen quad
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glEnable(GL_DEPTH_TEST);
}

void Application::renderBodies(const glm::mat4& view, const glm::mat4& projection)
{
    const glm::vec3 lightPosition = bodies_.empty()
        ? glm::vec3(0.0f)
        : bodies_[0].renderPosition();

    bodyShader_.use();
    bodyShader_.setMat4("view",       view);
    bodyShader_.setMat4("projection", projection);
    bodyShader_.setMat4("model",      glm::mat4(1.0f));
    bodyShader_.setVec3("lightPos",   lightPosition);
    bodyShader_.setVec3("viewPos",    camera_.getPosition());

    for (size_t i = 0; i < bodies_.size(); ++i)
    {
        bodies_[i].render(bodyShader_, uiManager_.isHovered(i));
    }
}

void Application::renderGrid(const glm::mat4& view, const glm::mat4& projection)
{
    gridShader_.use();
    gridShader_.setMat4("view",       view);
    gridShader_.setMat4("projection", projection);
    gridShader_.setMat4("model",      glm::mat4(1.0f));
    grid_.draw(gridShader_, bodies_);
}

void Application::renderRings(const glm::mat4& view, const glm::mat4& projection)
{
    ringShader_.use();
    ringShader_.setMat4("view",       view);
    ringShader_.setMat4("projection", projection);
    ringShader_.setVec3("ringColor",  kSaturnRingColor);

    glBindVertexArray(ringVAO_);
    glDepthMask(GL_FALSE);
    for (const auto& body : bodies_)
    {
        if (!body.hasRings) continue;
        const float displayRadius = body.radius * body.displayScale;
        const float ringScale     = displayRadius * kRingScaleMultiplier;
        glm::mat4 ringModel = glm::translate(glm::mat4(1.0f), body.renderPosition());
        ringModel = glm::scale(ringModel, glm::vec3(ringScale));
        ringShader_.setMat4("model", ringModel);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, ringVertexCount_);
    }
    glDepthMask(GL_TRUE);
    glBindVertexArray(0);
}

void Application::renderTrails(const glm::mat4& view, const glm::mat4& projection)
{
    trailShader_.use();
    trailShader_.setMat4("view",       view);
    trailShader_.setMat4("projection", projection);

    glBindVertexArray(trailVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, trailVBO_);
    glDepthMask(GL_FALSE);

    std::vector<glm::vec3> trailUploadBuffer;
    for (const auto& body : bodies_)
    {
        if (body.trailPoints.size() < 2) continue;
        trailUploadBuffer.assign(body.trailPoints.begin(), body.trailPoints.end());
        glBufferData(GL_ARRAY_BUFFER,
                     trailUploadBuffer.size() * sizeof(glm::vec3),
                     trailUploadBuffer.data(),
                     GL_DYNAMIC_DRAW);
        trailShader_.setVec3("trailColor", body.color);
        trailShader_.setInt ("pointCount", static_cast<int>(trailUploadBuffer.size()));
        glDrawArrays(GL_LINE_STRIP, 0, static_cast<GLsizei>(trailUploadBuffer.size()));
    }
    glDepthMask(GL_TRUE);
    glBindVertexArray(0);
}

void Application::renderHalos(const glm::mat4& view, const glm::mat4& projection)
{
    haloShader_.use();
    haloShader_.setMat4("view",       view);
    haloShader_.setMat4("projection", projection);

    glBindVertexArray(haloVAO_);
    glBlendFunc(GL_ONE, GL_ONE);   // additive
    glDepthMask(GL_FALSE);

    for (const auto& body : bodies_)
    {
        if (body.emissive < 0.5f) continue;
        const float displayRadius = body.radius * body.displayScale;
        haloShader_.setVec3 ("center", body.renderPosition());
        haloShader_.setFloat("size",   displayRadius * kHaloSizeMultiplier);
        haloShader_.setVec3 ("color",  body.color);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    glDepthMask(GL_TRUE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  // restore default
    glBindVertexArray(0);
}
