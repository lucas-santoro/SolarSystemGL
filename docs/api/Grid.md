# `Grid`

`src/core/Grid.h`

The flat XZ-plane grid that gets depressed in y by per-body mass — a visual stand-in for spacetime curvature.

## Definition

```cpp
class Grid {
public:
    Grid(float size, int divisions, float height = 0.0f);
    ~Grid();

    void setupGrid(float size, int divisions, float height);
    void draw(Shader& shader, const std::vector<CelestialBody>& bodies) const;

private:
    GLuint VAO, VBO;
    int    lineCount;
    std::vector<glm::vec3> originalPoints;
};
```

## Setup

The constructor calls `setupGrid(10000.0f, 200, 0.0f)`. That produces a 10 000 × 10 000 WU grid with 200 divisions per axis (so 400 lines in each direction, ≈ 160 000 vertices). Each line segment is two `vec3`s in the VBO.

`originalPoints` is built but never re-used — it was relevant to a CPU-side distortion implementation that has since moved to the shader. It's tracked under P1 (Code cleanup) for removal.

## Per-frame draw

```cpp
void draw(Shader& shader, const std::vector<CelestialBody>& bodies) const {
    int count = std::min(static_cast<int>(bodies.size()), 10);
    std::vector<glm::vec3> positions(count);
    std::vector<float>     masses(count);
    for (int i = 0; i < count; ++i) {
        positions[i] = bodies[i].renderPosition();
        masses[i]    = static_cast<float>(bodies[i].mass_kg / 5.97e24);
    }
    shader.setInt("planetCount", count);
    shader.setVec3Array("planetPositions", positions);
    shader.setFloatArray("planetMasses", masses);
    /* glDrawArrays(GL_LINES, ...) */
}
```

Masses are normalized by Earth (5.97 × 10²⁴ kg). Sun comes out at ~3.33 × 10⁵, Jupiter at ~318, etc. The shader's depression formula is tuned around this scale.

The 10-body hard cap is enforced here (`std::min(..., 10)`) and again in `GridVertexShader.glsl` (`#define MAX_PLANETS 10`). Increase both if you want more.

## Shader

The vertex shader (`shaders/GridVertexShader.glsl`) sums per-body depressions:

```glsl
for (int i = 0; i < planetCount; ++i) {
    vec2 delta = aPos.xz - planetPositions[i].xz;
    float dist2 = max(dot(delta, delta), 0.001);
    float depth = 0.008 * planetMasses[i] / (1.0 + dist2 / 4.0);
    distortedPos.y -= clamp(depth, 0.0, 38.0);
}
```

The falloff is `1 / (1 + r² / 4)` — Lorentzian, not Newtonian. The exact shape isn't physical; it just reads well as gravity.

Magic numbers (`0.008`, `4.0`, `38.0`) are calibrated by eye. Tuning them is part of the visual P1 backlog.

The fragment shader is a flat `vec4(1, 1, 1, 0.2)` — relies on the alpha blending state set once at startup (`glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)`).

## Resource lifecycle

`Grid` owns raw `VAO` / `VBO` handles but has no Rule of 5 — it relies on the implicit copy ctor, which would double-free on copy. Today `Grid` lives as a single local in `main()`, so it never gets copied; but this is a latent bug. Tracked under P1 (Code quality: Rule of 5 on Grid).
