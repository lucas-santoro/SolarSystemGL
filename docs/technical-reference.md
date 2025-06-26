# Technical Reference

This document provides comprehensive technical details about the SolarSystemGL implementation, including mathematical formulas, algorithms, and implementation specifics.

## 🧮 Physics Engine

### Gravitational Force Calculation

The simulation uses Newton's law of universal gravitation:

```
F = G * (m₁ * m₂) / r²
```

Where:
- `F` = Gravitational force (N)
- `G` = Gravitational constant = 6.67430×10⁻¹¹ m³/kg/s²
- `m₁, m₂` = Masses of the two bodies (kg)
- `r` = Distance between centers of mass (m)

### N-Body Simulation Algorithm

The physics system implements a direct N-body simulation using the following steps:

1. **Force Calculation** (O(N²) complexity):
```cpp
for (size_t i = 0; i < bodies.size(); ++i) {
    glm::dvec3 acc(0.0);
    for (size_t j = 0; j < bodies.size(); ++j) {
        if (i == j) continue;
        
        glm::dvec3 r = bodies[j].pos_m - bodies[i].pos_m;
        double dist2 = glm::dot(r, r) + SOFTEN;
        double invD = 1.0 / sqrt(dist2);
        
        acc += (G * bodies[j].mass_kg * invD * invD) * r * invD;
    }
    bodies[i].vel_m += acc * dtSim;
}
```

2. **Softening Parameter**:
```
SOFTEN = 1×10³ m²
```
This prevents singularities when objects get too close:
```
r_effective = sqrt(r² + ε²)
```

3. **Time Integration** (Leapfrog/Verlet method):
```
v(t + dt) = v(t) + a(t) * dt
x(t + dt) = x(t) + v(t + dt) * dt
```

### Time Scaling

Real time is accelerated by a factor of 860,400:
```
dt_simulation = dt_real * 860400
```
This corresponds to 10 days per second of real time.

### Unit Conversions

The simulation uses multiple unit systems:

1. **Physics Units** (SI):
   - Position: meters (m)
   - Velocity: meters per second (m/s)
   - Mass: kilograms (kg)

2. **Rendering Units** (World Units):
   - 1 WU = 1×10⁹ m = 1 Gm
   - Astronomical Unit: 1 AU = 149.6 Gm ≈ 149.6 WU

3. **Conversion Formula**:
```cpp
constexpr double METERS_PER_WU = 1.0e9;
glm::vec3 renderPos = glm::vec3(bodies[i].pos_m / METERS_PER_WU);
```

## 🎨 Rendering Pipeline

### 3D Sphere Generation

Planets are rendered as subdivided icosahedrons for optimal sphere approximation.

#### Initial Icosahedron Vertices

Using the golden ratio φ = (1 + √5)/2:
```cpp
float t = (1.0f + sqrt(5.0f)) / 2.0f; // φ ≈ 1.618

vertices = {
    (-1, t, 0), (1, t, 0), (-1, -t, 0), (1, -t, 0),
    (0, -1, t), (0, 1, t), (0, -1, -t), (0, 1, -t),
    (t, 0, -1), (t, 0, 1), (-t, 0, -1), (-t, 0, 1)
};
```

#### Subdivision Algorithm

Each subdivision level quadruples the triangle count:
```
Triangles = 20 * 4^subdivisions
Vertices = 10 * 4^subdivisions + 2
```

For subdivision level 3:
- Triangles: 20 * 64 = 1,280
- Vertices: 642

#### Vertex Normalization

Each vertex is projected to sphere surface:
```cpp
for (auto& v : vertices) {
    v = glm::normalize(v) * radius;
}
```

### Planet Radius Calculation

Radius is derived from mass and density:
```
Volume = mass / density
Volume_sphere = (4/3) * π * r³

Therefore: r = ∛(3 * mass / (4 * π * density))
```

With visual scaling factor:
```cpp
const float scaleFactor = 1e-7f;
radius = std::cbrt((3.0f * mass) / (4.0f * M_PI * density)) * scaleFactor;
```

### Camera System

#### View Matrix Calculation

The camera uses a look-at matrix:
```
View = lookAt(position, position + front, up)
```

Where:
- `position`: Camera world position
- `front`: Forward direction vector
- `up`: Up direction vector (0, 1, 0)

#### Projection Matrix

Perspective projection with:
```cpp
glm::mat4 projection = glm::perspective(
    glm::radians(45.0f),  // FOV = 45°
    aspect_ratio,         // width/height
    0.01f,               // Near plane
    10000.0f             // Far plane
);
```

#### Mouse Ray Casting

Screen to world ray conversion:
```cpp
glm::vec3 Camera::getRayFromMouse(double mouseX, double mouseY, 
                                  int screenWidth, int screenHeight,
                                  const glm::mat4& view, 
                                  const glm::mat4& projection) {
    // Normalize mouse coordinates to [-1, 1]
    float x = (2.0f * mouseX) / screenWidth - 1.0f;
    float y = 1.0f - (2.0f * mouseY) / screenHeight;
    
    // Create ray in clip space
    glm::vec4 rayClip = glm::vec4(x, y, -1.0f, 1.0f);
    
    // Transform to eye space
    glm::vec4 rayEye = glm::inverse(projection) * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
    
    // Transform to world space
    glm::vec3 rayWorld = glm::vec3(glm::inverse(view) * rayEye);
    return glm::normalize(rayWorld);
}
```

### Ray-Sphere Intersection

For planet selection, ray-sphere intersection uses the quadratic formula:

Given ray: `P(t) = origin + t * direction`
Sphere: `|P - center|² = radius²`

Solving: `at² + bt + c = 0`

Where:
```
a = |direction|²
b = 2 * dot(origin - center, direction)
c = |origin - center|² - radius²
```

Discriminant: `Δ = b² - 4ac`
- If Δ ≥ 0: intersection exists
- If Δ < 0: no intersection

```cpp
bool Planet::intersectsRay(const glm::vec3& rayOrigin, 
                          const glm::vec3& rayDirection) const {
    float pickRadius = std::max(getRadius(), MIN_PICK_RADIUS);
    glm::vec3 originToCtr = rayOrigin - position;
    
    float dirLenSq = glm::dot(rayDirection, rayDirection);
    float twiceProj = 2.0f * glm::dot(originToCtr, rayDirection);
    float centerDistSq = glm::dot(originToCtr, originToCtr) - 
                        pickRadius * pickRadius;
    
    float discriminant = twiceProj * twiceProj - 
                        4.0f * dirLenSq * centerDistSq;
    
    return discriminant >= 0.0f;
}
```

## 🌐 Grid System

### Grid Distortion Shader

The spacetime grid visualizes gravitational field distortion using the vertex shader:

```glsl
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

#define MAX_PLANETS 10
uniform vec3 planetPositions[MAX_PLANETS];
uniform float planetMasses[MAX_PLANETS];
uniform int planetCount;

void main() {
    vec3 distortedPos = aPos;
    
    for (int i = 0; i < planetCount; ++i) {
        vec2 delta = distortedPos.xz - planetPositions[i].xz;
        float dist2 = max(dot(delta, delta), 0.001);
        
        // Gravitational potential approximation
        float distortion = 0.008 * planetMasses[i] / (1.0 + dist2 / (2.0 * 2.0));
        distortion = clamp(distortion, 0.0, 38.0);
        
        distortedPos.y -= distortion;
    }
    
    gl_Position = projection * view * model * vec4(distortedPos, 1.0);
}
```

### Grid Generation

The grid is created as a series of lines:
```cpp
void Grid::setupGrid(float size, int divisions, float height) {
    std::vector<glm::vec3> vertices;
    float step = size / divisions;
    float half = size * 0.5f;
    
    // Horizontal lines
    for (int i = 0; i <= divisions; ++i) {
        float z = -half + i * step;
        vertices.push_back(glm::vec3(-half, height, z));
        vertices.push_back(glm::vec3(half, height, z));
    }
    
    // Vertical lines
    for (int i = 0; i <= divisions; ++i) {
        float x = -half + i * step;
        vertices.push_back(glm::vec3(x, height, -half));
        vertices.push_back(glm::vec3(x, height, half));
    }
    
    lineCount = vertices.size();
}
```

## 📊 Performance Analysis

### Computational Complexity

1. **Physics Update**: O(N²) where N = number of planets
2. **Rendering**: O(V) where V = total vertices across all planets
3. **UI Update**: O(N) for planet list management

### Memory Usage

For N planets with subdivision level S:
```
Vertices per planet = 10 * 4^S + 2
Total vertices = N * (10 * 4^S + 2)
Memory per vertex = 3 * sizeof(float) = 12 bytes
Total geometry memory ≈ N * (10 * 4^S + 2) * 12 bytes
```

For default configuration (9 planets, subdivision 3):
```
Memory ≈ 9 * 642 * 12 ≈ 69 KB
```

### Frame Rate Optimization

Target 60 FPS requires frame time ≤ 16.67 ms:
- Physics: ~2-3 ms
- Rendering: ~10-12 ms
- UI: ~1-2 ms
- Overhead: ~2-3 ms

## 🔧 Shader Programs

### Vertex Shader (Planets)
```glsl
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
```

### Fragment Shader (Planets)
```glsl
#version 330 core
out vec4 FragColor;

uniform vec3 planetColor;

void main() {
    FragColor = vec4(planetColor, 1.0);
}
```

### Fragment Shader (Grid)
```glsl
#version 330 core
out vec4 FragColor;

void main() {
    FragColor = vec4(1.0, 1.0, 1.0, 0.2); // Semi-transparent white
}
```

## 🎛️ Configuration Constants

### Physics Constants
```cpp
static constexpr double G = 6.67430e-11;        // m³/kg/s²
static constexpr double SOFTEN = 1e3;           // m²
double timeScale = 860'400.0;                   // dimensionless
```

### Rendering Constants
```cpp
static constexpr float MIN_PICK_RADIUS = 18.0f; // pixels
const float scaleFactor = 1e-7f;                // dimensionless
constexpr double METERS_PER_WU = 1.0e9;         // m/WU
constexpr double AU = 1.495978707e11;           // m
```

### OpenGL Settings
```cpp
glEnable(GL_DEPTH_TEST);
glEnable(GL_BLEND);
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
```

## 🔍 Debugging Features

### Debug Visualization
```cpp
// Uncommented code in Planet::render() shows planet hitboxes
if (highlight) {
    float pickRadius = std::max(radius, MIN_PICK_RADIUS) * visualScale;
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    // Render wireframe sphere
}
```

### Performance Monitoring
```cpp
ImGui::Text("FPS: %.1f", 1.0f / deltaTime);
```

### Memory Profiling
- Debug builds include memory leak detection
- Release builds optimize for performance

## 📐 Mathematical Derivations

### Orbital Velocity Calculation

For circular orbits, centripetal force equals gravitational force:
```
mv²/r = GMm/r²
v = √(GM/r)
```

Initial velocities are set based on this formula:
```cpp
addBody(1.000, 29780.0, 5.9720e24); // Earth: v ≈ √(GM☉/1AU)
```

### Escape Velocity
```
v_escape = √(2GM/r)
```

### Orbital Period (Kepler's Third Law)
```
T² = (4π²/GM) * a³
```

Where:
- T = orbital period
- a = semi-major axis
- M = central mass

---

**This technical reference provides the mathematical foundation for understanding and extending the SolarSystemGL simulation.** 