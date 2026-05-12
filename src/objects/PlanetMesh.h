#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

/**
 * @file PlanetMesh.h
 * @brief OpenGL state for a subdivided-icosahedron sphere mesh.
 */

/**
 * @brief Owns the VAO/VBO/EBO and CPU-side geometry of one body's sphere mesh.
 *
 * The constructor stores the subdivision count but does **not** touch OpenGL —
 * call #rebuild at least once (with the desired radius) before drawing. This
 * keeps the constructor safe to run before the GL context exists.
 *
 * Non-copyable. Movable: the move constructor transfers the GL handles and
 * nulls the source so its destructor becomes a no-op.
 */
class PlanetMesh
{
public:
    /**
     * @brief Construct without uploading geometry.
     * @param subdivisions Number of icosahedron subdivision rounds (3 → 1280 triangles).
     */
    explicit PlanetMesh(int subdivisions = 3);

    /** @brief Release the GL handles. */
    ~PlanetMesh();

    PlanetMesh(const PlanetMesh&)            = delete;
    PlanetMesh& operator=(const PlanetMesh&) = delete;
    PlanetMesh(PlanetMesh&& other) noexcept;
    PlanetMesh& operator=(PlanetMesh&& other) noexcept;

    /**
     * @brief Regenerate the geometry at a new radius and upload it to the GPU.
     *
     * Generates the base icosahedron, subdivides it (caching midpoints to share
     * vertices), and re-uploads positions + indices via `GL_STATIC_DRAW`.
     *
     * @param radius Sphere radius in world units.
     */
    void rebuild(float radius);

    /** @brief Issue a `glDrawElements(GL_TRIANGLES, ...)` for the mesh. */
    void draw() const;

private:
    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint EBO = 0;
    std::vector<glm::vec3>    vertices;
    std::vector<unsigned int> indices;
    int                       subdivisions;

    void releaseGL();
};
