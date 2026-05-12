# `PlanetMesh`

`src/objects/PlanetMesh.h`

Owns the OpenGL resources (`VAO`, `VBO`, `EBO`) and the CPU-side mesh data for one body's subdivided icosahedron. Non-copyable, movable, with explicit Rule of 5.

## Definition

```cpp
class PlanetMesh {
public:
    explicit PlanetMesh(int subdivisions = 3);
    ~PlanetMesh();

    PlanetMesh(const PlanetMesh&)            = delete;
    PlanetMesh& operator=(const PlanetMesh&) = delete;
    PlanetMesh(PlanetMesh&&) noexcept;
    PlanetMesh& operator=(PlanetMesh&&) noexcept;

    void rebuild(float radius);
    void draw() const;

private:
    GLuint VAO = 0, VBO = 0, EBO = 0;
    std::vector<glm::vec3>    vertices;
    std::vector<unsigned int> indices;
    int                       subdivisions;

    void releaseGL();
};
```

## Why Rule of 5 explicitly?

The class owns raw OpenGL handles. The default copy operations would copy the handle integers — two `PlanetMesh` instances pointing at the same VAO. The first to destruct would `glDeleteVertexArrays` the buffers; the second would do it again and either crash or corrupt the driver's state.

The chosen rule:
- **Copy ctor / assign**: `delete`d. Bodies cannot be copied; this also forces `CelestialBody` to be move-only, which surfaces accidental copy attempts at compile time.
- **Move ctor / assign**: defined explicitly. Transfer the handles, zero them in the source so the source's destructor is a no-op.
- **Destructor**: calls `releaseGL()`, which `glDelete*`s only handles ≠ 0.

This is what makes `std::vector<CelestialBody>` work safely: `push_back` and reallocation move the meshes around without re-uploading geometry or leaking handles.

## Construction is GL-free

```cpp
PlanetMesh::PlanetMesh(int subdivisions)
    : subdivisions(subdivisions) {}
```

The constructor only stores the subdivision count. **No GL calls.** This means you can default-construct a `PlanetMesh` (and thus a `CelestialBody`) before the GL context exists, which is convenient for `bodies.reserve(16)` + initialization loop patterns.

The first `rebuild()` does the upload.

## `rebuild(float radius)`

Re-runs the mesh generation pipeline:
1. Reset to the base 12 icosahedron vertices, normalized and scaled by `radius`.
2. Apply `subdivisions` rounds of triangle subdivision, using an edge-midpoint cache (`std::map<pair, index>`) to avoid duplicating shared midpoints.
3. Generate `VAO`/`VBO`/`EBO` if not already present.
4. Upload positions and indices via `GL_STATIC_DRAW`.

For 3 subdivisions: 12 → 42 → 162 → 642 vertices; 20 → 80 → 320 → 1280 triangles.

`recalculateGeometry()` on `CelestialBody` calls `rebuild(radius)` after editing mass or density.

## `draw() const`

```cpp
glBindVertexArray(VAO);
glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, nullptr);
glBindVertexArray(0);
```

The EBO is bound through the VAO, so the `nullptr` is the offset into the bound element buffer, not a CPU pointer.

## Vertex layout

Currently a single `vec3` attribute at location 0 (position). The vertex shader derives the surface normal from `normalize(aPos)`, exploiting that all vertices lie on a sphere centered at the origin in model space.

When textures are added (P1), the layout will grow to `(vec3 position, vec2 uv)` and `setupMesh` will interleave them.
