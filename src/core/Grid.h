#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include "objects/CelestialBody.h"

/**
 * @file Grid.h
 * @brief Translucent line grid that bends in y by body mass — and the
 *        `GridSettings` bag that controls every visual / physical knob.
 */

/**
 * @brief Aggregate of every grid-tunable parameter.
 *
 * The bag is owned by `Application`, mutated by the Settings modal and the
 * bottom actionbar's Grid toggle, and read by `Grid::draw` each frame.
 * Geometry-affecting fields (`style`, `resolution`, `extent`) are detected
 * by `Grid::rebuildIfDirty` and trigger a VBO rebuild on change; the rest
 * are uploaded as shader uniforms.
 */
struct GridSettings
{
    /// @brief Two grid topologies — Cartesian rows × columns or Radial rings × spokes.
    enum class Style { Cartesian, Radial };

    // -- Visibility --------------------------------------------------------

    /// Master toggle. When `false`, `Application::renderGrid` skips the call entirely.
    bool      visible              = true;

    // -- Visual ------------------------------------------------------------

    /// RGB of grid lines at zero depth (flat regions).
    glm::vec3 baseColor            = { 1.0f, 1.0f, 1.0f };

    /// RGB the lines blend toward as the well depth grows (used by Bundle B).
    glm::vec3 wellColor            = { 0.20f, 0.05f, 0.40f };

    /// Base line alpha applied before per-line / distance modulation.
    float     opacity              = 0.20f;

    /// Alpha multiplier for "major" lines (every Nth row/ring). Bundle B.
    float     majorLineBoost       = 2.5f;

    /// Major-line stride. `1` makes every line major (no minor lines). Bundle B.
    int       majorLineInterval    = 10;

    /// Distance from the camera at which the alpha starts ramping toward zero. Bundle B.
    float     distanceFadeStart    = 4000.0f;

    /// Distance from the camera at which the alpha reaches zero. Bundle B.
    float     distanceFadeEnd      = 9000.0f;

    // -- Distortion physics -----------------------------------------------

    /// Multiplier in the falloff numerator (`depth = strength × mass / …`).
    float     distortionStrength   = 0.008f;

    /// Falloff radius used by the default Lorentzian profile.
    float     falloffRadius        = 2.0f;

    /// Hard upper bound on per-body well depth in world units.
    float     maxWellDepth         = 38.0f;

    /// When true, the shader uses a Schwarzschild-inspired `1/r` profile
    /// instead of the smooth Lorentzian. Bundle C.
    bool      useSchwarzschild     = false;

    /// Strength of the "singularity" darkening applied to very deep wells. Bundle C.
    float     singularityDarken    = 1.0f;

    /// Mix the well color with the per-body color contributing to that well. Bundle C.
    bool      perBodyTint          = true;

    // -- Geometry ----------------------------------------------------------

    Style     style                = Style::Cartesian;

    /// Cartesian: half the line count per axis. Radial: ring count.
    int       resolution           = 200;

    /// Total side length (Cartesian) or outer radius diameter (Radial), in world units.
    float     extent               = 10000.0f;
};

/**
 * @brief Spacetime-curvature grid drawn beneath the bodies.
 *
 * A flat lattice (Cartesian rows × columns by default, optionally radial)
 * on the XZ plane. The vertex shader displaces each point in y by a per-body
 * mass-weighted falloff, evoking the classic "rubber sheet" intuition for
 * gravitational wells.
 *
 * The mesh is rebuilt only when geometry-affecting `GridSettings` fields
 * change (style, resolution, extent). Every other frame, `draw` just uploads
 * uniforms — no CPU geometry work.
 *
 * Non-copyable, movable (RAII for the GL handles).
 */
class Grid
{
public:
    /**
     * @brief Build the initial grid mesh on the GPU.
     * @param size      Total side length of the grid in world units (also stored as last-built extent).
     * @param divisions Cells per axis (stored as last-built resolution).
     * @param height    Unused; kept for source compatibility.
     */
    Grid(float size, int divisions, float height = 0.0f);

    /** @brief Release the GL handles. */
    ~Grid();

    Grid(const Grid&)            = delete;
    Grid& operator=(const Grid&) = delete;
    Grid(Grid&& other) noexcept;
    Grid& operator=(Grid&& other) noexcept;

    /**
     * @brief Issue the grid draw call with current per-body uniforms.
     *
     * Triggers a VBO rebuild before drawing if any of `settings.style`,
     * `settings.resolution`, or `settings.extent` changed since the last
     * call.
     *
     * @param shader   Bound grid shader.
     * @param bodies   Current body list (capped by `MAX_PLANETS` on both sides).
     * @param settings Active GridSettings — visual + physical + geometry knobs.
     */
    void draw(Shader& shader,
              const std::vector<CelestialBody>& bodies,
              const GridSettings& settings);

private:
    void buildCartesianGeometry(int divisions, float extent);
    void rebuildIfDirty(const GridSettings& settings);

    GLuint VAO       = 0;
    GLuint VBO       = 0;
    int    lineCount = 0;

    // Last-built geometry signature — used to detect when a rebuild is needed.
    GridSettings::Style lastBuiltStyle_      = GridSettings::Style::Cartesian;
    int                 lastBuiltResolution_ = 0;
    float               lastBuiltExtent_     = 0.0f;
};
