#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include "objects/CelestialBody.h"

/**
 * @file Grid.h
 * @brief Translucent XZ-plane line grid that bends in y by body mass.
 */

/**
 * @brief Spacetime-curvature grid drawn beneath the bodies.
 *
 * A flat lattice of line segments on the XZ plane. The vertex shader displaces
 * each point in y by a per-body mass-weighted Lorentzian falloff, evoking the
 * classic "rubber sheet" intuition for gravitational wells.
 *
 * The mesh is built once at construction. Each frame, #draw uploads the
 * current per-body positions and Earth-normalized masses as uniform arrays.
 * Non-copyable, movable (RAII for the GL handles).
 */
class Grid
{
public:
    /**
     * @brief Build the grid mesh on the GPU.
     * @param size      Total side length of the grid in world units.
     * @param divisions Number of cells per axis (vertex count is roughly `divisions * 2 + 1` per axis).
     * @param height    Unused; kept for source compatibility.
     */
    Grid(float size, int divisions, float height = 0.0f);

    /** @brief Release the GL handles. */
    ~Grid();

    Grid(const Grid&)            = delete;
    Grid& operator=(const Grid&) = delete;
    Grid(Grid&& other) noexcept;
    Grid& operator=(Grid&& other) noexcept;

    /** @brief Rebuild the grid mesh with new parameters. */
    void setupGrid(float size, int divisions, float height);

    /**
     * @brief Issue the grid draw call with current per-body uniforms.
     * @param shader Bound grid shader receiving `planetPositions`, `planetMasses`, `planetCount`.
     * @param bodies Current body list (capped at 10 entries by both the client and the shader).
     */
    void draw(Shader& shader, const std::vector<CelestialBody>& bodies) const;

private:
    GLuint VAO       = 0;
    GLuint VBO       = 0;
    int    lineCount = 0;
};
