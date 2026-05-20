#include <glad/glad.h>
#include "core/Grid.h"
#include <algorithm>
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

void Grid::rebuildIfDirty(const GridSettings& settings)
{
    const bool styleChanged      = settings.style      != lastBuiltStyle_;
    const bool resolutionChanged = settings.resolution != lastBuiltResolution_;
    const bool extentChanged     = settings.extent     != lastBuiltExtent_;
    if (!styleChanged && !resolutionChanged && !extentChanged) return;

    // Phase 1: only Cartesian is implemented. Phase 5 adds the Radial branch.
    buildCartesianGeometry(settings.resolution, settings.extent);

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
