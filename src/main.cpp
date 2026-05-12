#include <glad/glad.h>
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"

#include "core/Window.h"
#include "core/Shader.h"
#include "core/Camera.h"
#include "core/Grid.h"
#include "core/Constants.h"
#include "ui/UIManager.h"
#include "objects/CelestialBody.h"
#include "physics/PhysicsSystem.h"

#include <algorithm>
#include <vector>

namespace {

struct App {
    Camera     camera{ glm::vec3(0.0f, 0.0f, 300.0f) };
    UIManager  uiManager;
    float      lastX      = 400.0f;
    float      lastY      = 300.0f;
    bool       firstMouse = true;
    bool       prevSpace  = false;
    bool       prevR      = false;
    bool       prevF      = false;
    bool       prevEsc    = false;
    bool       prevN1     = false;
    bool       prevN2     = false;
};

App* appFromWindow(GLFWwindow* w) {
    return static_cast<App*>(glfwGetWindowUserPointer(w));
}

void processInput(GLFWwindow* w, Camera& camera, float deltaTime) {
    // Note: Esc is handled in the edge-detected hotkey block (two-stage: deselect, then close).
    if (glfwGetKey(w, GLFW_KEY_W) == GLFW_PRESS) camera.processKeyboard(GLFW_KEY_W, deltaTime);
    if (glfwGetKey(w, GLFW_KEY_S) == GLFW_PRESS) camera.processKeyboard(GLFW_KEY_S, deltaTime);
    if (glfwGetKey(w, GLFW_KEY_A) == GLFW_PRESS) camera.processKeyboard(GLFW_KEY_A, deltaTime);
    if (glfwGetKey(w, GLFW_KEY_D) == GLFW_PRESS) camera.processKeyboard(GLFW_KEY_D, deltaTime);
}

void mouseCallback(GLFWwindow* w, double xpos, double ypos) {
    App* app = appFromWindow(w);
    if (!app) return;

    if (app->firstMouse) {
        app->lastX = static_cast<float>(xpos);
        app->lastY = static_cast<float>(ypos);
        app->firstMouse = false;
    }
    const float xoffset = static_cast<float>(xpos) - app->lastX;
    const float yoffset = app->lastY - static_cast<float>(ypos);
    app->lastX = static_cast<float>(xpos);
    app->lastY = static_cast<float>(ypos);

    if (app->uiManager.isRightMousePressed(w))
        app->camera.processMouseMovement(xoffset, yoffset);
}

void scrollCallback(GLFWwindow* w, double /*xoffset*/, double yoffset) {
    App* app = appFromWindow(w);
    if (!app || ImGui::GetIO().WantCaptureMouse) return;
    app->camera.processKeyboard(yoffset > 0 ? GLFW_KEY_W : GLFW_KEY_S, 0.1f);
}

} // namespace

int main()
{
    Window window(800, 600, "SolarSystemGL");
    App    app;
    glfwSetWindowUserPointer(window.getGLFWwindow(), &app);

    glfwSetFramebufferSizeCallback(window.getGLFWwindow(),
        [](GLFWwindow*, int width, int height) { glViewport(0, 0, width, height); });
    glfwSetCursorPosCallback(window.getGLFWwindow(), mouseCallback);
    glfwSetScrollCallback(window.getGLFWwindow(), scrollCallback);
    glfwSetInputMode(window.getGLFWwindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Shader shader     ("shaders/VertexShader.glsl",       "shaders/FragmentShader.glsl");
    Shader gridShader ("shaders/GridVertexShader.glsl",   "shaders/GridFragmentShader.glsl");
    Shader trailShader("shaders/TrailVertexShader.glsl",  "shaders/TrailFragmentShader.glsl");
    Shader haloShader ("shaders/HaloVertexShader.glsl",   "shaders/HaloFragmentShader.glsl");
    Shader skyShader  ("shaders/SkyVertexShader.glsl",    "shaders/SkyFragmentShader.glsl");
    Shader ringShader ("shaders/RingVertexShader.glsl",   "shaders/RingFragmentShader.glsl");
    PhysicsSystem physics;

    // Dynamic VBO for trail line strips — re-uploaded per body per frame.
    GLuint trailVAO = 0, trailVBO = 0;
    glGenVertexArrays(1, &trailVAO);
    glGenBuffers(1, &trailVBO);
    glBindVertexArray(trailVAO);
    glBindBuffer(GL_ARRAY_BUFFER, trailVBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Static unit quad for billboarded halo passes (one quad re-used for every star).
    GLuint haloVAO = 0, haloVBO = 0;
    {
        const float quadVerts[] = {
            -1.0f, -1.0f,
             1.0f, -1.0f,
            -1.0f,  1.0f,
             1.0f,  1.0f,
        };
        glGenVertexArrays(1, &haloVAO);
        glGenBuffers(1, &haloVBO);
        glBindVertexArray(haloVAO);
        glBindBuffer(GL_ARRAY_BUFFER, haloVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    constexpr size_t TRAIL_CAPACITY = 512;
    constexpr float  MIN_TRAIL_STEP = 0.2f;
    constexpr float  TRAIL_JUMP     = 100.0f;  // > this WU jump → clear trail (teleport)

    // Ring mesh: triangle-strip ring on XZ plane, inner radius 1.0, outer radius 1.5.
    constexpr int    RING_SEGMENTS = 64;
    GLuint           ringVAO = 0, ringVBO = 0;
    GLsizei          ringVertCount = 0;
    {
        std::vector<glm::vec3> ringVerts;
        ringVerts.reserve(static_cast<size_t>(2 * (RING_SEGMENTS + 1)));
        for (int i = 0; i <= RING_SEGMENTS; ++i) {
            const float angle = 2.0f * 3.14159265358979f * static_cast<float>(i) / RING_SEGMENTS;
            const float cx = std::cos(angle);
            const float sz = std::sin(angle);
            ringVerts.emplace_back(cx * 1.0f, 0.0f, sz * 1.0f);
            ringVerts.emplace_back(cx * 1.5f, 0.0f, sz * 1.5f);
        }
        ringVertCount = static_cast<GLsizei>(ringVerts.size());

        glGenVertexArrays(1, &ringVAO);
        glGenBuffers(1, &ringVBO);
        glBindVertexArray(ringVAO);
        glBindBuffer(GL_ARRAY_BUFFER, ringVBO);
        glBufferData(GL_ARRAY_BUFFER,
                     ringVerts.size() * sizeof(glm::vec3),
                     ringVerts.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    std::vector<CelestialBody> bodies;
    bodies.reserve(16); // keep capacity stable for "Add Planet" up to 16 bodies

    struct Init {
        const char* name;
        double      a_au;
        double      v_ms;
        double      mass_kg;
        float       density;
        glm::vec3   color;
        float       emissive;
        float       displayScale;   // visual-only model scale (Sun shrunk so it doesn't engulf orbits)
        bool        hasRings;
    };

    const Init initList[] = {
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

    for (const auto& init : initList) {
        CelestialBody body;
        body.pos_m        = glm::dvec3(init.a_au * AU, 0.0, 0.0);
        body.vel_m        = glm::dvec3(0.0, 0.0, init.v_ms);
        body.mass_kg      = init.mass_kg;
        body.name         = init.name;
        body.color        = init.color;
        body.density      = init.density;
        body.emissive     = init.emissive;
        body.displayScale = init.displayScale;
        body.hasRings     = init.hasRings;
        body.recalculateGeometry();
        bodies.push_back(std::move(body));
    }

    // Center-of-mass correction: shift the whole system into the barycenter rest frame.
    // Without this, the Sun-as-origin convention leaves total momentum non-zero, and the
    // barycenter drifts secularly over long runs.
    {
        glm::dvec3 totalMomentum(0.0);
        double     totalMass = 0.0;
        for (const auto& b : bodies) {
            totalMomentum += b.vel_m * b.mass_kg;
            totalMass     += b.mass_kg;
        }
        if (totalMass > 0.0) {
            const glm::dvec3 vCom = totalMomentum / totalMass;
            for (auto& b : bodies) b.vel_m -= vCom;
        }
    }

    Grid grid(10000.0f, 200, 0.0f);

    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window.getGLFWwindow(), true);
    ImGui_ImplOpenGL3_Init("#version 330");

    glfwSwapInterval(app.uiManager.vsync ? 1 : 0);  // initial VSync state (default on)

    float lastFrame = 0.0f;

    while (!glfwWindowShouldClose(window.getGLFWwindow()))
    {
        const float currentFrame = static_cast<float>(glfwGetTime());
        const float deltaTime    = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Refresh orbital target position from current body position (index is stable, address may not be).
        const int orbIdx = app.camera.getOrbitalTargetIndex();
        if (orbIdx >= 0 && orbIdx < static_cast<int>(bodies.size())) {
            app.camera.setOrbitalTargetPos(bodies[orbIdx].renderPosition());
        }

        app.camera.update(deltaTime);
        processInput(window.getGLFWwindow(), app.camera, deltaTime);

        // Edge-detected hotkeys. Skipped while an ImGui text field has keyboard focus
        // (so typing in a field doesn't fire shortcuts).
        {
            GLFWwindow* w = window.getGLFWwindow();
            const bool kbCaptured = ImGui::GetIO().WantCaptureKeyboard;
            const bool spaceNow   = glfwGetKey(w, GLFW_KEY_SPACE)  == GLFW_PRESS;
            const bool rNow       = glfwGetKey(w, GLFW_KEY_R)      == GLFW_PRESS;
            const bool fNow       = glfwGetKey(w, GLFW_KEY_F)      == GLFW_PRESS;
            const bool escNow     = glfwGetKey(w, GLFW_KEY_ESCAPE) == GLFW_PRESS;
            const bool n1Now      = glfwGetKey(w, GLFW_KEY_1)      == GLFW_PRESS;
            const bool n2Now      = glfwGetKey(w, GLFW_KEY_2)      == GLFW_PRESS;

            if (!kbCaptured) {
                if (spaceNow && !app.prevSpace) physics.paused = !physics.paused;
                if (rNow     && !app.prevR)     app.camera.reset();

                if (fNow && !app.prevF) {
                    const int sel = app.uiManager.getSelectedPlanetIndex();
                    if (sel >= 0 && sel < static_cast<int>(bodies.size())) {
                        const float dist = std::max(50.0f,
                            bodies[sel].radius * bodies[sel].displayScale * 4.0f);
                        app.camera.startSmoothMove(bodies[sel].renderPosition(), dist);
                    }
                }

                if (escNow && !app.prevEsc) {
                    // Two-stage Esc: close panel first, only close app if nothing is selected.
                    if (app.uiManager.getSelectedPlanetIndex() >= 0) {
                        app.uiManager.clearSelection();
                    } else {
                        glfwSetWindowShouldClose(w, true);
                    }
                }

                if (n1Now && !app.prevN1) app.camera.setMode(CameraMode::FREE);
                if (n2Now && !app.prevN2) {
                    const int sel = app.uiManager.getSelectedPlanetIndex();
                    if (sel >= 0 && sel < static_cast<int>(bodies.size())) {
                        const float dist = std::max(50.0f,
                            bodies[sel].radius * bodies[sel].displayScale * 4.0f);
                        app.camera.setMode(CameraMode::ORBITAL);
                        app.camera.setOrbitalTarget(sel, dist);
                    }
                }
            }

            app.prevSpace = spaceNow;
            app.prevR     = rNow;
            app.prevF     = fNow;
            app.prevEsc   = escNow;
            app.prevN1    = n1Now;
            app.prevN2    = n2Now;
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        int width, height;
        glfwGetFramebufferSize(window.getGLFWwindow(), &width, &height);
        const glm::mat4 view       = app.camera.getViewMatrix();
        const glm::mat4 projection = glm::perspective(glm::radians(45.0f),
                                                      static_cast<float>(width) / static_cast<float>(std::max(height, 1)),
                                                      0.01f, 10000.0f);
        const glm::mat4 model = glm::mat4(1.0f);

        // 0) Skybox / starfield — first, depth-test off. Anything else overdraws.
        skyShader.use();
        skyShader.setMat4("invView", glm::inverse(view));
        skyShader.setMat4("invProj", glm::inverse(projection));
        glDisable(GL_DEPTH_TEST);
        glBindVertexArray(haloVAO);   // reuse the existing fullscreen quad
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glEnable(GL_DEPTH_TEST);

        physics.update(bodies, static_cast<double>(deltaTime));

        // Sample trails: push current render position if moved enough; clear on teleport.
        for (auto& body : bodies) {
            const glm::vec3 pos = body.renderPosition();
            if (!body.trailPoints.empty()) {
                const float jump = glm::distance(body.trailPoints.back(), pos);
                if (jump > TRAIL_JUMP) body.trailPoints.clear();
            }
            if (body.trailPoints.empty()
                || glm::distance(body.trailPoints.back(), pos) > MIN_TRAIL_STEP)
            {
                body.trailPoints.push_back(pos);
                if (body.trailPoints.size() > TRAIL_CAPACITY) body.trailPoints.pop_front();
            }
        }

        // Lighting uniforms — computed AFTER physics so the Sun's position is current.
        const glm::vec3 lightPos = bodies.empty() ? glm::vec3(0.0f) : bodies[0].renderPosition();

        // 1) Opaque planets.
        shader.use();
        shader.setMat4("view", view);
        shader.setMat4("projection", projection);
        shader.setMat4("model", model);
        shader.setVec3("lightPos", lightPos);
        shader.setVec3("viewPos",  app.camera.getPosition());
        for (size_t i = 0; i < bodies.size(); ++i)
            bodies[i].render(shader, app.uiManager.isHovered(i));

        // 2) Spacetime grid (translucent).
        gridShader.use();
        gridShader.setMat4("view", view);
        gridShader.setMat4("projection", projection);
        gridShader.setMat4("model", glm::mat4(1.0f));
        grid.draw(gridShader, bodies);

        // 2b) Planetary rings (translucent disks for bodies with `hasRings`).
        ringShader.use();
        ringShader.setMat4("view", view);
        ringShader.setMat4("projection", projection);
        ringShader.setVec3("ringColor", glm::vec3(0.90f, 0.85f, 0.70f));
        glBindVertexArray(ringVAO);
        glDepthMask(GL_FALSE);
        for (const auto& body : bodies) {
            if (!body.hasRings) continue;
            const float displayRadius = body.radius * body.displayScale;
            const float ringScale     = displayRadius * 1.5f;
            glm::mat4 ringModel = glm::translate(glm::mat4(1.0f), body.renderPosition());
            ringModel = glm::scale(ringModel, glm::vec3(ringScale));
            ringShader.setMat4("model", ringModel);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, ringVertCount);
        }
        glDepthMask(GL_TRUE);
        glBindVertexArray(0);

        // 3) Orbit trails (translucent line strips) — depth write off so they don't occlude.
        if (app.uiManager.showTrails)
        {
            trailShader.use();
            trailShader.setMat4("view", view);
            trailShader.setMat4("projection", projection);
            glBindVertexArray(trailVAO);
            glBindBuffer(GL_ARRAY_BUFFER, trailVBO);
            glDepthMask(GL_FALSE);

            std::vector<glm::vec3> tmp;
            for (const auto& body : bodies) {
                if (body.trailPoints.size() < 2) continue;
                tmp.assign(body.trailPoints.begin(), body.trailPoints.end());
                glBufferData(GL_ARRAY_BUFFER,
                             tmp.size() * sizeof(glm::vec3),
                             tmp.data(),
                             GL_DYNAMIC_DRAW);
                trailShader.setVec3("trailColor", body.color);
                trailShader.setInt("pointCount", static_cast<int>(tmp.size()));
                glDrawArrays(GL_LINE_STRIP, 0, static_cast<GLsizei>(tmp.size()));
            }
            glDepthMask(GL_TRUE);
            glBindVertexArray(0);
        }

        // 4) Star halos (additive billboards) — only emissive bodies.
        if (app.uiManager.showBloom)
        {
            haloShader.use();
            haloShader.setMat4("view", view);
            haloShader.setMat4("projection", projection);
            glBindVertexArray(haloVAO);
            glBlendFunc(GL_ONE, GL_ONE);
            glDepthMask(GL_FALSE);

            for (const auto& body : bodies) {
                if (body.emissive < 0.5f) continue;
                const float renderRadius = body.radius * body.displayScale;
                haloShader.setVec3 ("center", body.renderPosition());
                haloShader.setFloat("size",   renderRadius * 3.0f);
                haloShader.setVec3 ("color",  body.color);
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            }
            glDepthMask(GL_TRUE);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glBindVertexArray(0);
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        app.uiManager.render(window, app.camera, deltaTime, bodies, grid, physics);

        if (app.uiManager.vsyncDirty) {
            glfwSwapInterval(app.uiManager.vsync ? 1 : 0);
            app.uiManager.vsyncDirty = false;
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window.getGLFWwindow());
        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    return 0;
}
