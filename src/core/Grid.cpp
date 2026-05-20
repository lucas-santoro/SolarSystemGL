#include <glad/glad.h>
#include "core/Grid.h"
#include <algorithm>
#include <cmath>
#include <utility>

namespace
{
    /// Mass divisor — same value the original implementation used to express
    /// each body in Earth-equivalent units before sending to the shader.
    constexpr double kEarthMassKg = 5.97e24;

    /// Max bodies that can influence the grid simultaneously. Mirrored as
    /// `MAX_PLANETS` in the grid vertex shader; both must stay in sync.
    constexpr int kMaxPlanetsForGrid = 32;
}

Grid::Grid(float size, int divisions, float /*height*/)
{
    buildCartesianGeometry(divisions, size);
    lastBuiltStyle_      = GridSettings::Style::Cartesian;
    lastBuiltResolution_ = divisions;
    lastBuiltExtent_     = size;
}

Grid::~Grid()
{
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
}

Grid::Grid(Grid&& other) noexcept
    : VAO(other.VAO)
    , VBO(other.VBO)
    , lineCount(other.lineCount)
    , lastBuiltStyle_(other.lastBuiltStyle_)
    , lastBuiltResolution_(other.lastBuiltResolution_)
    , lastBuiltExtent_(other.lastBuiltExtent_)
{
    other.VAO = 0;
    other.VBO = 0;
}

Grid& Grid::operator=(Grid&& other) noexcept
{
    if (this != &other)
    {
        if (VAO) glDeleteVertexArrays(1, &VAO);
        if (VBO) glDeleteBuffers(1, &VBO);
        VAO                  = other.VAO;
        VBO                  = other.VBO;
        lineCount            = other.lineCount;
        lastBuiltStyle_      = other.lastBuiltStyle_;
        lastBuiltResolution_ = other.lastBuiltResolution_;
        lastBuiltExtent_     = other.lastBuiltExtent_;
        other.VAO            = 0;
        other.VBO            = 0;
    }
    return *this;
}

void Grid::buildCartesianGeometry(int divisions, float extent)
{
    const int   actualDivisions = divisions * 2;
    const float step            = extent / actualDivisions;
    const float half            = extent / 2.0f;

    std::vector<glm::vec3> gridPoints;
    gridPoints.reserve(static_cast<size_t>(actualDivisions + 1) * (actualDivisions + 1));
    for (int i = 0; i <= actualDivisions; ++i)
    {
        for (int j = 0; j <= actualDivisions; ++j)
        {
            const float x = -half + j * step;
            const float z = -half + i * step;
            gridPoints.emplace_back(x, 0.0f, z);
        }
    }

    // Interleaved vertex layout: (x, y, z, lineIndex) — 4 floats per vertex.
    // lineIndex carries the row index for horizontal lines and the column
    // index for vertical lines; the shader uses it for major/minor banding.
    std::vector<GLfloat> vertices;
    vertices.reserve(gridPoints.size() * 8);

    // Horizontal lines (row i, columns j to j+1).
    for (int i = 0; i <= actualDivisions; ++i)
    {
        const float lineIndex = static_cast<float>(i);
        for (int j = 0; j < actualDivisions; ++j)
        {
            const int   idx = i * (actualDivisions + 1) + j;
            const auto& a   = gridPoints[idx];
            const auto& b   = gridPoints[idx + 1];
            vertices.insert(vertices.end(), {
                a.x, a.y, a.z, lineIndex,
                b.x, b.y, b.z, lineIndex,
            });
        }
    }

    // Vertical lines (column j, rows i to i+1).
    for (int j = 0; j <= actualDivisions; ++j)
    {
        const float lineIndex = static_cast<float>(j);
        for (int i = 0; i < actualDivisions; ++i)
        {
            const int   idx     = i       * (actualDivisions + 1) + j;
            const int   nextIdx = (i + 1) * (actualDivisions + 1) + j;
            const auto& a       = gridPoints[idx];
            const auto& b       = gridPoints[nextIdx];
            vertices.insert(vertices.end(), {
                a.x, a.y, a.z, lineIndex,
                b.x, b.y, b.z, lineIndex,
            });
        }
    }

    constexpr int kFloatsPerVertex = 4;
    lineCount = static_cast<int>(vertices.size()) / kFloatsPerVertex;

    if (VAO == 0) glGenVertexArrays(1, &VAO);
    if (VBO == 0) glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat),
                 vertices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          kFloatsPerVertex * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE,
                          kFloatsPerVertex * sizeof(GLfloat),
                          (void*)(3 * sizeof(GLfloat)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Grid::buildRadialGeometry(int divisions, float extent)
{
    // Radial layout: a stack of concentric rings + a set of radial spokes,
    // both tessellated finely enough to look smooth and to deform cleanly
    // around bodies passing through them.
    const int   rings           = std::max(divisions / 4, 8);
    const int   spokes          = std::clamp(divisions / 4, 16, 96);
    constexpr int kRingSegments = 64;
    const float maxRadius       = extent * 0.5f;

    constexpr int   kFloatsPerVertex = 4;
    constexpr float kTwoPi           = 6.283185307179586f;

    std::vector<GLfloat> vertices;
    vertices.reserve(
        static_cast<size_t>(rings) * kRingSegments * 2 * kFloatsPerVertex +
        static_cast<size_t>(spokes) * rings * 2 * kFloatsPerVertex);

    // Concentric rings.
    for (int ring = 0; ring < rings; ++ring)
    {
        const float radius    = maxRadius * static_cast<float>(ring + 1) / static_cast<float>(rings);
        const float lineIndex = static_cast<float>(ring);
        for (int seg = 0; seg < kRingSegments; ++seg)
        {
            const float a0 = kTwoPi * static_cast<float>(seg)     / static_cast<float>(kRingSegments);
            const float a1 = kTwoPi * static_cast<float>(seg + 1) / static_cast<float>(kRingSegments);
            const float x0 = radius * std::cos(a0);
            const float z0 = radius * std::sin(a0);
            const float x1 = radius * std::cos(a1);
            const float z1 = radius * std::sin(a1);
            vertices.insert(vertices.end(), {
                x0, 0.0f, z0, lineIndex,
                x1, 0.0f, z1, lineIndex,
            });
        }
    }

    // Radial spokes — each broken at every ring intersection so the
    // distortion bends smoothly along the spoke instead of in a single
    // straight segment from origin to the outer radius.
    for (int spoke = 0; spoke < spokes; ++spoke)
    {
        const float angle     = kTwoPi * static_cast<float>(spoke) / static_cast<float>(spokes);
        const float dirX      = std::cos(angle);
        const float dirZ      = std::sin(angle);
        const float lineIndex = static_cast<float>(spoke);
        for (int seg = 0; seg < rings; ++seg)
        {
            const float r0 = maxRadius * static_cast<float>(seg)     / static_cast<float>(rings);
            const float r1 = maxRadius * static_cast<float>(seg + 1) / static_cast<float>(rings);
            vertices.insert(vertices.end(), {
                r0 * dirX, 0.0f, r0 * dirZ, lineIndex,
                r1 * dirX, 0.0f, r1 * dirZ, lineIndex,
            });
        }
    }

    lineCount = static_cast<int>(vertices.size()) / kFloatsPerVertex;

    if (VAO == 0) glGenVertexArrays(1, &VAO);
    if (VBO == 0) glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat),
                 vertices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          kFloatsPerVertex * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE,
                          kFloatsPerVertex * sizeof(GLfloat),
                          (void*)(3 * sizeof(GLfloat)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Grid::rebuildIfDirty(const GridSettings& settings)
{
    const bool styleChanged      = settings.style      != lastBuiltStyle_;
    const bool resolutionChanged = settings.resolution != lastBuiltResolution_;
    const bool extentChanged     = settings.extent     != lastBuiltExtent_;
    if (!styleChanged && !resolutionChanged && !extentChanged) return;

    switch (settings.style)
    {
        case GridSettings::Style::Radial:
            buildRadialGeometry(settings.resolution, settings.extent);
            break;
        case GridSettings::Style::Cartesian:
        default:
            buildCartesianGeometry(settings.resolution, settings.extent);
            break;
    }

    lastBuiltStyle_      = settings.style;
    lastBuiltResolution_ = settings.resolution;
    lastBuiltExtent_     = settings.extent;
}

void Grid::draw(Shader& shader,
                const std::vector<CelestialBody>& bodies,
                const GridSettings& settings)
{
    rebuildIfDirty(settings);

    shader.use();

    const int count = std::min(static_cast<int>(bodies.size()), kMaxPlanetsForGrid);
    std::vector<glm::vec3> positions(count);
    std::vector<float>     masses(count);
    std::vector<glm::vec3> colors(count);

    for (int i = 0; i < count; ++i)
    {
        positions[i] = bodies[i].renderPosition();
        masses[i]    = static_cast<float>(bodies[i].mass_kg / kEarthMassKg);
        colors[i]    = bodies[i].color;
    }

    shader.setInt("planetCount", count);
    shader.setVec3Array("planetPositions", positions);
    shader.setFloatArray("planetMasses",   masses);
    shader.setVec3Array("planetColors",    colors);

    // Distortion uniforms.
    shader.setFloat("gridStrength",     settings.distortionStrength);
    shader.setFloat("falloffRadius",    settings.falloffRadius);
    shader.setFloat("maxWellDepth",     settings.maxWellDepth);
    shader.setInt  ("useSchwarzschild", settings.useSchwarzschild ? 1 : 0);

    // Visual uniforms.
    shader.setVec3 ("baseColor",         settings.baseColor);
    shader.setVec3 ("wellColor",         settings.wellColor);
    shader.setFloat("opacity",           settings.opacity);
    shader.setFloat("majorLineBoost",    settings.majorLineBoost);
    shader.setInt  ("majorLineInterval", settings.majorLineInterval);
    shader.setFloat("distanceFadeStart", settings.distanceFadeStart);
    shader.setFloat("distanceFadeEnd",   settings.distanceFadeEnd);
    shader.setInt  ("perBodyTint",       settings.perBodyTint ? 1 : 0);
    shader.setFloat("singularityDarken", settings.singularityDarken);

    glBindVertexArray(VAO);
    glDrawArrays(GL_LINES, 0, lineCount);
    glBindVertexArray(0);
}
