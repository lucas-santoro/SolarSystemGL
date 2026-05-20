#pragma once

#include <glad/glad.h>
#include <vector>

#include "core/Camera.h"
#include "core/Framebuffer.h"
#include "core/Grid.h"
#include "core/Shader.h"
#include "core/Window.h"
#include "objects/CelestialBody.h"
#include "objects/PlanetMesh.h"
#include "physics/PathPredictor.h"
#include "physics/PhysicsSystem.h"
#include "ui/AddPlanetModal.h"
#include "ui/SaveLoadModal.h"
#include "ui/SettingsModal.h"
#include "ui/StartMenu.h"
#include "ui/UIManager.h"

/**
 * @file Application.h
 * @brief Top-level application object: owns the window, GL resources, simulation
 *        state, and the main loop.
 */

/**
 * @brief Discrete lifecycle phase of the application.
 *
 * The main loop branches on this enum so the menu screen and the running
 * simulation can share the window/GL context but render independently.
 */
enum class AppState
{
    Menu,    ///< Pre-simulation: rotating skybox + StartMenu modal.
    Running  ///< Simulation is active: physics, bodies, full HUD.
};

/**
 * @brief Owns the entire SolarSystemGL application lifecycle.
 *
 * Construction initializes GLFW + OpenGL, compiles every shader, builds the
 * default solar system, and creates the auxiliary GL buffers (trail / halo /
 * ring). #run blocks on the main loop until the user closes the window.
 *
 * The class is intentionally **not** copyable or movable: it owns a GLFW
 * window (and indirectly the OpenGL context) plus several GL handles whose
 * ownership cannot be safely transferred.
 *
 * Construction can throw `std::runtime_error` (from `Shader`) if any shader
 * file is missing or fails to compile.
 */
class Application
{
public:
    /**
     * @brief Construct an application instance and initialize all subsystems.
     *
     * @param windowWidth  Initial window width in pixels.
     * @param windowHeight Initial window height in pixels.
     * @param title        Window title shown by the OS / window manager.
     *
     * @throws std::runtime_error if GLFW or any shader fails to initialize.
     */
    Application(int windowWidth, int windowHeight, const char* title);

    /**
     * @brief Releases auxiliary GL buffers and shuts down ImGui.
     */
    ~Application();

    Application(const Application&)            = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&)                 = delete;
    Application& operator=(Application&&)      = delete;

    /**
     * @brief Enter the main loop. Returns when the user closes the window.
     */
    void run();

private:
    /// Push the default 9-body Solar System into @ref bodies_.
    void loadDefaultBodies();

    /// Subtract total-momentum-over-total-mass from each body so the
    /// barycenter starts at rest. Idempotent on already-centered systems.
    void shiftToBarycenterFrame();

    /// Build the dynamic trail VBO, the static halo quad, and the static ring mesh.
    void createAuxiliaryBuffers();

    /// Release the auxiliary GL buffers. Called from the destructor.
    void destroyAuxiliaryBuffers();

    /// One iteration of the main loop.
    void tick();

    /// Draw the menu screen for one frame (skybox rotation + StartMenu modal).
    void renderMenuFrame(float deltaTime);

    /// Build the default solar system and transition to AppState::Running.
    void startSimulation();

    /// Tear down the running simulation and transition back to AppState::Menu.
    void returnToMenu();

    /// Refresh @ref camera_'s cached orbital target position from the current
    /// body the camera is tracking by index.
    void refreshOrbitalCameraTarget();

    /// Process edge-detected one-shot hotkeys (Space, R, F, Esc, 1, 2, F12).
    void handleHotkeys();

    /// Read the current framebuffer and save it to `screenshots/<timestamp>.bmp`.
    void captureScreenshot();

    /// Append the current frame's body positions to their trail history,
    /// applying motion threshold and teleport-detection rules.
    void sampleOrbitTrails();

    /// Procedural starfield — first pass, fills the background.
    void renderSky(const glm::mat4& view, const glm::mat4& projection);

    /// Opaque planet meshes with lighting uniforms.
    void renderBodies(const glm::mat4& view, const glm::mat4& projection);

    /// Translucent spacetime-curvature grid.
    void renderGrid(const glm::mat4& view, const glm::mat4& projection);

    /// Procedural ring disks for any body with `hasRings == true`.
    void renderRings(const glm::mat4& view, const glm::mat4& projection);

    /// Translucent rim-lit shell around any body with `hasAtmosphere == true`.
    void renderAtmospheres(const glm::mat4& view, const glm::mat4& projection);

    /// Five small glowing markers at L1–L5 between the Sun and the selected
    /// body. Toggle via `uiManager_.showLagrange`.
    void renderLagrange(const glm::mat4& view, const glm::mat4& projection);

    /// Per-body orbit-history line strips with alpha fade.
    void renderTrails(const glm::mat4& view, const glm::mat4& projection);

    /// Reuses the trail shader to draw the path predictor's cached line strip.
    void renderPathPrediction(const glm::mat4& view, const glm::mat4& projection);

    /// Render emissive bodies into the bloom ping FBO and run ping-pong
    /// Gaussian blurs. Leaves the final blurred image in @p outBlurredTexture.
    void renderBloomPasses(const glm::mat4& view, const glm::mat4& projection,
                           GLuint& outBlurredTexture);

    /// Fullscreen quad pass: composite `sceneFbo_.colorTexture()` with the
    /// blurred bloom result onto the default framebuffer.
    void renderComposite(GLuint blurredBloomTexture, int viewportWidth, int viewportHeight);

    /// GLFW cursor-position callback — dispatches to the Application instance
    /// stored in the window's user-pointer.
    static void mouseCallback(GLFWwindow* window, double xpos, double ypos);

    /// GLFW scroll callback — dispatches to the Application instance.
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);

    /// GLFW window-focus callback — auto-pauses the simulation while the
    /// window is in the background. Resumes only if the auto-pause put it
    /// there (manual pauses are not undone).
    static void windowFocusCallback(GLFWwindow* window, int focused);

    /// GLFW window-close callback — intercepts the close event in Running
    /// state so the user can confirm the quit; lets it through in Menu.
    static void windowCloseCallback(GLFWwindow* window);

    /// GLFW framebuffer-size callback — resizes the viewport and all owned FBOs.
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);

    /// Draw any active confirm modals (Return to Menu? / Quit application?).
    /// Must be invoked between ImGui::NewFrame and ImGui::Render.
    void renderConfirmModals();

    /// Retrieve the Application instance attached to the GLFW window.
    static Application* fromWindow(GLFWwindow* window);

    // Construction order matters: Window first (creates the GL context),
    // then GL-owning resources (Shaders, Grid, Framebuffers), then non-GL state.
    Window        window_;
    Camera        camera_;
    Shader        bodyShader_;
    Shader        gridShader_;
    Shader        trailShader_;
    Shader        skyShader_;
    Shader        ringShader_;
    Shader        bloomBlurShader_;
    Shader        bloomCompositeShader_;
    Shader        atmosphereShader_;
    Framebuffer   sceneFbo_;
    Framebuffer   bloomPingFbo_;
    Framebuffer   bloomPongFbo_;
    PhysicsSystem physics_;
    PathPredictor pathPredictor_;
    UIManager       uiManager_;
    StartMenu       startMenu_;
    SettingsModal   settingsModal_;
    SaveLoadModal   saveLoadModal_;
    AddPlanetModal  addPlanetModal_;
    Grid            grid_;
    GridSettings    gridSettings_;
    std::vector<CelestialBody> bodies_;

    // User-tunable rendering settings (bound to the Settings modal).
    float fieldOfView_ = 45.0f;
    float guiScale_    = 1.0f;

    // Lifecycle state.
    AppState appState_                  = AppState::Menu;
    float    menuTime_                  = 0.0f;
    bool     wasAutoPaused_             = false;  ///< Set when the window-focus callback auto-pauses physics.
    bool     openReturnToMenuPopup_     = false;  ///< Pending "Return to Menu?" modal open request.
    bool     openQuitPopup_             = false;  ///< Pending "Quit application?" modal open request.

    // Auxiliary GL resources directly owned by Application.
    GLuint  trailVAO_            = 0;
    GLuint  trailVBO_            = 0;
    GLuint  ringVAO_             = 0;
    GLuint  ringVBO_             = 0;
    GLuint  fullscreenQuadVAO_   = 0;  ///< Shared by the sky pass and the bloom fullscreen passes.
    GLuint  fullscreenQuadVBO_   = 0;
    GLsizei ringVertexCount_     = 0;

    PlanetMesh lagrangeMesh_{ 2 };       ///< Low-poly icosahedron used for L1–L5 markers.

    // Mouse-input state, shared between the GLFW callbacks and the main loop.
    float lastMouseX_       = 400.0f;
    float lastMouseY_       = 300.0f;
    bool  firstMouseEvent_  = true;

    // Edge-detection state for one-shot keyboard hotkeys.
    bool wasSpacePressed_ = false;
    bool wasRPressed_     = false;
    bool wasFPressed_     = false;
    bool wasEscPressed_   = false;
    bool wasOnePressed_   = false;
    bool wasTwoPressed_   = false;
    bool wasF12Pressed_   = false;

    // Real-time tracking for delta-time computation.
    float previousFrameTime_ = 0.0f;

    // Path-prediction throttle: only recompute every N frames.
    int   framesSinceLastPrediction_ = 0;
};
