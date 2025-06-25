# Shader Reference

Complete documentation of all OpenGL shaders used in SolarSystemGL, including GLSL code, uniforms, and rendering pipeline details.

## 🎨 Rendering Pipeline Overview

The application uses a forward rendering pipeline with two main shader programs:

1. **Planet Shader**: Renders spherical celestial bodies
2. **Grid Shader**: Renders the spacetime grid with gravitational distortion

### Pipeline Flow

```
Vertex Data → Vertex Shader → Primitive Assembly → Rasterization → Fragment Shader → Frame Buffer
```

**Key Features:**
- OpenGL 3.3 Core Profile
- GLSL 330 shaders
- Perspective projection
- Depth testing enabled
- Alpha blending for grid transparency

## 🌍 Planet Rendering System

### Planet Vertex Shader
*File: `shaders/VertexShader.glsl`*

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

**Input Attributes:**
- `aPos` (location 0): Vertex position in local space (vec3)

**Uniforms:**
- `model`: Model-to-world transformation matrix (mat4)
- `view`: World-to-camera transformation matrix (mat4)
- `projection`: Camera-to-clip space transformation matrix (mat4)

**Transformation Pipeline:**
1. Local space → World space (model matrix)
2. World space → Camera space (view matrix)
3. Camera space → Clip space (projection matrix)

### Planet Fragment Shader
*File: `shaders/FragmentShader.glsl`*

```glsl
#version 330 core
out vec4 FragColor;

uniform vec3 planetColor;

void main()
{
    FragColor = vec4(planetColor, 1.0);
}
```

**Uniforms:**
- `planetColor`: RGB color values (vec3, range 0.0-1.0)

**Output:**
- `FragColor`: Final pixel color with full opacity (vec4)

**Rendering Features:**
- Flat shading (no lighting calculation)
- Solid colors for clear planet identification
- Full opacity (alpha = 1.0)

## 🌐 Grid Rendering System

### Grid Vertex Shader
*File: `shaders/GridVertexShader.glsl`*

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

**Input Attributes:**
- `aPos` (location 0): Grid vertex position in local space (vec3)

**Standard Uniforms:**
- `model`, `view`, `projection`: Transformation matrices (mat4)

**Physics Uniforms:**
- `planetPositions[MAX_PLANETS]`: Array of planet world positions (vec3[10])
- `planetMasses[MAX_PLANETS]`: Array of normalized planet masses (float[10])
- `planetCount`: Number of active planets (int, ≤ 10)

**Distortion Algorithm:**

1. **Distance Calculation:**
   ```glsl
   vec2 delta = distortedPos.xz - planetPositions[i].xz;
   float dist2 = max(dot(delta, delta), 0.001);
   ```
   - Uses XZ plane distance (2D)
   - Minimum distance prevents division by zero

2. **Gravitational Potential:**
   ```glsl
   float distortion = 0.008 * planetMasses[i] / (1.0 + dist2 / (2.0 * 2.0));
   ```
   - Approximates gravitational potential: φ ∝ M/r
   - Scale factor: 0.008 (tuned for visual effect)
   - Smoothing factor: (1.0 + dist2/16.0)

3. **Distortion Application:**
   ```glsl
   distortion = clamp(distortion, 0.0, 38.0);
   distortedPos.y -= distortion;
   ```
   - Clamps maximum distortion to prevent artifacts
   - Applies downward displacement (negative Y)

### Grid Fragment Shader
*File: `shaders/GridFragmentShader.glsl`*

```glsl
#version 330 core
out vec4 FragColor;

void main()
{
    FragColor = vec4(1.0, 1.0, 1.0, 0.2);
}
```

**Output:**
- `FragColor`: Semi-transparent white (RGBA: 1.0, 1.0, 1.0, 0.2)

**Visual Properties:**
- White color for clear visibility
- 20% opacity (alpha = 0.2)
- Requires alpha blending: `glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)`

## 🔧 Shader Management

### Shader Class Implementation
*File: `src/core/Shader.h`*

The Shader class provides a C++ wrapper for OpenGL shader programs:

```cpp
class Shader {
public:
    Shader(const std::string& vertexPath, const std::string& fragmentPath);
    
    void use();
    void setMat4(const std::string& name, const glm::mat4& mat);
    void setVec3(const std::string& name, const glm::vec3& value);
    void setVec3Array(const std::string& name, const std::vector<glm::vec3>& values);
    void setFloatArray(const std::string& name, const std::vector<float>& values);
    void setInt(const std::string& name, int value);
    void setFloat(const std::string& name, float value);
};
```

### Uniform Setting Examples

**Matrix Uniforms:**
```cpp
shader.setMat4("model", modelMatrix);
shader.setMat4("view", camera.getViewMatrix());
shader.setMat4("projection", projectionMatrix);
```

**Vector Uniforms:**
```cpp
shader.setVec3("planetColor", glm::vec3(1.0f, 0.9f, 0.3f)); // Sun color
```

**Array Uniforms:**
```cpp
std::vector<glm::vec3> positions;
std::vector<float> masses;
for (const auto& planet : planets) {
    positions.push_back(planet->getPosition());
    masses.push_back(planet->getMass() / 5.97e24f); // Normalize to Earth masses
}
gridShader.setVec3Array("planetPositions", positions);
gridShader.setFloatArray("planetMasses", masses);
gridShader.setInt("planetCount", positions.size());
```

## 📐 Mathematical Foundations

### Coordinate Transformations

**Model Matrix:**
```
M = T × R × S
```
Where:
- T: Translation matrix
- R: Rotation matrix  
- S: Scaling matrix

**View Matrix (Look-At):**
```
V = [ Right.x   Right.y   Right.z   -dot(Right, Eye)   ]
    [ Up.x      Up.y      Up.z      -dot(Up, Eye)      ]
    [ -Front.x  -Front.y  -Front.z   dot(Front, Eye)   ]
    [ 0         0         0          1                  ]
```

**Projection Matrix (Perspective):**
```
P = [ f/aspect  0        0                    0                  ]
    [ 0         f        0                    0                  ]
    [ 0         0        -(far+near)/(far-near)  -2*far*near/(far-near) ]
    [ 0         0        -1                   0                  ]
```
Where f = cot(FOV/2)

### Physics Visualization Mathematics

**Gravitational Potential (Simplified):**
```
φ(r) = -GM/r
```

**Grid Distortion Function:**
```
y_distorted = y_original - Σᵢ (αMᵢ)/(1 + r²ᵢ/β²)
```

Where:
- α = 0.008 (visual scaling factor)
- β = 2.0 (smoothing parameter)
- Mᵢ = normalized mass of planet i
- rᵢ = horizontal distance to planet i

## 🎛️ OpenGL State Configuration

### Required OpenGL Settings

```cpp
// Depth testing
glEnable(GL_DEPTH_TEST);
glDepthFunc(GL_LESS);

// Alpha blending (for grid transparency)
glEnable(GL_BLEND);
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

// Viewport
glViewport(0, 0, width, height);

// Clear values
glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Black background
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
```

### Vertex Array Object Setup

**Planet VAO Configuration:**
```cpp
glGenVertexArrays(1, &VAO);
glGenBuffers(1, &VBO);
glGenBuffers(1, &EBO);

glBindVertexArray(VAO);

glBindBuffer(GL_ARRAY_BUFFER, VBO);
glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), 
             vertexData.data(), GL_STATIC_DRAW);

glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), 
             indices.data(), GL_STATIC_DRAW);

// Position attribute (location 0)
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
glEnableVertexAttribArray(0);
```

**Grid VAO Configuration:**
```cpp
glGenVertexArrays(1, &VAO);
glGenBuffers(1, &VBO);
glBindVertexArray(VAO);
glBindBuffer(GL_ARRAY_BUFFER, VBO);
glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), 
             vertices.data(), GL_STATIC_DRAW);

// Position attribute (location 0)
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)0);
glEnableVertexAttribArray(0);
```

## 🔍 Debugging and Optimization

### Shader Compilation Error Checking

```cpp
void checkCompileErrors(unsigned int shader, std::string type) {
    int success;
    char infoLog[1024];
    
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            std::cerr << "Shader compilation error (" << type << "):\n" 
                      << infoLog << std::endl;
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            std::cerr << "Program linking error:\n" << infoLog << std::endl;
        }
    }
}
```

### Performance Considerations

**Uniform Updates:**
- Update uniforms only when necessary
- Batch uniform updates before drawing
- Use uniform buffer objects for large datasets

**Vertex Data:**
- Use GL_STATIC_DRAW for planet geometry (rarely changes)
- Minimize vertex attribute size
- Optimize vertex cache utilization

**Draw Calls:**
- Minimize state changes between draws
- Group similar objects together
- Use instanced rendering for multiple similar objects

### Common Shader Issues

**Grid Artifacts:**
- Ensure minimum distance in grid calculations
- Clamp distortion values to prevent overflow
- Check for NaN values in distance calculations

**Planet Rendering:**
- Verify matrix multiplication order
- Check for degenerate transformation matrices
- Ensure proper depth testing

**Performance Problems:**
- Monitor fragment shader complexity
- Limit loop iterations in shaders
- Profile GPU usage with graphics debugger

## 📊 Shader Performance Metrics

### Computational Complexity

**Planet Vertex Shader:** O(1) per vertex
- Simple matrix multiplications
- No loops or branching

**Grid Vertex Shader:** O(N) per vertex where N = planet count
- Loop over all planets for distortion calculation
- Maximum 10 iterations per vertex

**Fragment Shaders:** O(1) per fragment
- Simple color output
- No complex calculations

### Memory Usage

**Planet Shader:**
- 3 × mat4 uniforms = 192 bytes
- 1 × vec3 uniform = 12 bytes
- Total: ~204 bytes per shader

**Grid Shader:**
- 3 × mat4 uniforms = 192 bytes
- 10 × vec3 array = 120 bytes
- 10 × float array = 40 bytes
- 1 × int uniform = 4 bytes
- Total: ~356 bytes per shader

---

**This shader reference provides complete technical details for understanding and modifying the OpenGL rendering pipeline in SolarSystemGL.** 