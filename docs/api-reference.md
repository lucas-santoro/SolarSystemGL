# API Reference

Complete reference for all classes, methods, and interfaces in SolarSystemGL.

## 📚 Core Classes

### Camera Class
*Location: `src/core/Camera.h`*

Manages 3D camera movement, view matrix calculation, and user input processing.

#### Constructor
```cpp
Camera(glm::vec3 startPosition)
```
**Parameters:**
- `startPosition`: Initial camera position in world coordinates

#### Public Methods

##### `glm::mat4 getViewMatrix() const`
Returns the current view matrix for rendering.
**Returns:** 4x4 view matrix

##### `glm::vec3 getRayFromMouse(double mouseX, double mouseY, int screenWidth, int screenHeight, const glm::mat4& view, const glm::mat4& projection)`
Converts screen mouse coordinates to world space ray.
**Parameters:**
- `mouseX`, `mouseY`: Mouse coordinates in screen space
- `screenWidth`, `screenHeight`: Screen dimensions in pixels
- `view`: Current view matrix
- `projection`: Current projection matrix
**Returns:** Normalized direction vector in world space

##### `glm::vec3 getPosition()`
**Returns:** Current camera position in world coordinates

##### `glm::vec2 worldToScreen(const glm::vec3& worldPos, const glm::mat4& view, const glm::mat4& projection, int screenWidth, int screenHeight)`
Projects world coordinates to screen space.
**Parameters:**
- `worldPos`: Position in world coordinates
- `view`: Current view matrix
- `projection`: Current projection matrix
- `screenWidth`, `screenHeight`: Screen dimensions
**Returns:** Screen coordinates (x, y)

##### `void processKeyboard(int key, float deltaTime)`
Handles keyboard input for camera movement.
**Parameters:**
- `key`: GLFW key code (GLFW_KEY_W, GLFW_KEY_S, etc.)
- `deltaTime`: Time since last frame in seconds

##### `void processMouseMovement(float xoffset, float yoffset)`
Handles mouse movement for camera rotation.
**Parameters:**
- `xoffset`: Horizontal mouse movement
- `yoffset`: Vertical mouse movement

##### `void startSmoothMove(const glm::vec3& destination, float distance = 60.0f)`
Initiates smooth camera movement to target position.
**Parameters:**
- `destination`: Target world position
- `distance`: Distance from target to stop at

##### `void update(float dt)`
Updates camera state, including smooth movement animation.
**Parameters:**
- `dt`: Delta time in seconds

#### Public Members
```cpp
glm::vec3 position;    // Camera world position
glm::vec3 front;       // Forward direction vector
glm::vec3 up;          // Up direction vector
float yaw;             // Horizontal rotation (degrees)
float pitch;           // Vertical rotation (degrees)
float speed;           // Movement speed
float sensitivity;     // Mouse sensitivity
```

---

### Planet Class
*Location: `src/objects/Planet.h`*

Represents a celestial body with physics properties and 3D rendering.

#### Constructor
```cpp
Planet(const std::string& name, float mass, float density, 
       glm::vec3 position, glm::vec3 velocity, glm::vec3 color, 
       int subdivisions = 3)
```
**Parameters:**
- `name`: Planet name for UI display
- `mass`: Mass in kilograms
- `density`: Density in kg/m³
- `position`: Initial position in world coordinates
- `velocity`: Initial velocity vector
- `color`: RGB color (0.0-1.0 range)
- `subdivisions`: Icosahedron subdivision level (detail)

#### Public Methods

##### `void render(Shader& shader, bool highlight = false)`
Renders the planet using the specified shader.
**Parameters:**
- `shader`: OpenGL shader program
- `highlight`: Whether to highlight planet (selection)

##### `bool intersectsRay(const glm::vec3& rayOrigin, const glm::vec3& rayDirection) const`
Tests ray-sphere intersection for mouse picking.
**Parameters:**
- `rayOrigin`: Ray start position
- `rayDirection`: Ray direction vector
**Returns:** True if ray intersects planet

##### Getters
```cpp
std::string getName() const;     // Planet name
glm::vec3 getPosition() const;   // World position
glm::vec3 getVelocity() const;   // Velocity vector
float getRadius() const;         // Calculated radius
float getMass() const;           // Mass in kg
float getDensity() const;        // Density in kg/m³
glm::vec3 getColor() const;      // RGB color
float getPickRadius() const;     // Mouse picking radius
```

##### Setters
```cpp
void setPosition(const glm::vec3& newPosition);
void setVelocity(const glm::vec3& newVelocity);
void setMass(float newMass);
void setDensity(float newDensity);
void setName(const std::string& newName);
```

##### `void recalculateGeometry()`
Regenerates planet mesh after property changes.

---

### PhysicsSystem Class
*Location: `src/physics/PhysicsSystem.h`*

Handles N-body gravitational simulation.

#### Public Members
```cpp
double timeScale = 860'400.0;                    // Time acceleration factor
static constexpr double G = 6.67430e-11;        // Gravitational constant
static constexpr double SOFTEN = 1e3;           // Softening parameter
```

#### Public Methods

##### `void update(std::vector<BodyState>& bodies, double dtReal) const`
Updates physics simulation for one timestep.
**Parameters:**
- `bodies`: Vector of celestial body states
- `dtReal`: Real-time delta in seconds

---

### Grid Class
*Location: `src/core/Grid.h`*

Manages the spacetime grid visualization with gravitational distortion.

#### Constructor
```cpp
Grid(float size, int divisions, float height = 0.0f)
```
**Parameters:**
- `size`: Grid size in world units
- `divisions`: Number of grid lines per axis
- `height`: Base height (Y coordinate)

#### Public Methods

##### `void setupGrid(float size, int divisions, float height)`
Initializes grid geometry.

##### `void draw(Shader& shader, const std::vector<std::shared_ptr<Planet>>& planets) const`
Renders the grid with gravitational distortion.
**Parameters:**
- `shader`: Grid shader program
- `planets`: Planets causing distortion

---

### Shader Class
*Location: `src/core/Shader.h`*

OpenGL shader program wrapper with utility methods.

#### Constructor
```cpp
Shader(const std::string& vertexPath, const std::string& fragmentPath)
```
**Parameters:**
- `vertexPath`: Path to vertex shader file
- `fragmentPath`: Path to fragment shader file

#### Public Methods

##### `void use()`
Activates this shader program for rendering.

##### Uniform Setters
```cpp
void setMat4(const std::string& name, const glm::mat4& mat);
void setVec3(const std::string& name, const glm::vec3& value);
void setVec3Array(const std::string& name, const std::vector<glm::vec3>& values);
void setFloatArray(const std::string& name, const std::vector<float>& values);
void setInt(const std::string& name, int value);
void setFloat(const std::string& name, float value);
```

---

### UIManager Class
*Location: `src/ui/UIManager.h`*

Manages all user interface elements and interactions.

#### Public Methods

##### `void render(Window& window, Camera& camera, float deltaTime, std::vector<std::shared_ptr<Planet>>& planets, Grid& grid)`
Renders all UI elements for one frame.
**Parameters:**
- `window`: Application window
- `camera`: Camera instance
- `deltaTime`: Frame time
- `planets`: Planet list
- `grid`: Grid instance

##### `bool isRightMousePressed(GLFWwindow* window)`
Checks if right mouse button is pressed (not captured by UI).
**Returns:** True if right mouse is pressed

##### `bool isHovered(size_t i) const`
Checks if planet at index i is currently hovered.
**Parameters:**
- `i`: Planet index
**Returns:** True if planet is hovered

#### Private Methods
```cpp
void renderPlanetPopup(Window& window, Camera& camera, 
                      const glm::mat4& view, const glm::mat4& projection,
                      const std::vector<std::shared_ptr<Planet>>& planets);
void renderPlanetInfo(std::shared_ptr<Planet>& planet);
void renderMainPanel(float deltaTime, std::vector<std::shared_ptr<Planet>>& planets, Grid& grid);
void renderNavbar(std::vector<std::shared_ptr<Planet>>& planets);
```

---

### Window Class
*Location: `src/core/Window.h`*

GLFW window wrapper with OpenGL context management.

#### Constructor
```cpp
Window(int width, int height, const std::string& title)
```
**Parameters:**
- `width`: Window width in pixels
- `height`: Window height in pixels
- `title`: Window title string

#### Public Methods

##### `GLFWwindow* getGLFWwindow()`
**Returns:** Raw GLFW window pointer

##### `void run()`
Basic render loop (not used in main application).

---

## 📊 Data Structures

### BodyState Struct
*Location: `src/physics/BodyState.h`*

Represents physical state of a celestial body.

```cpp
struct BodyState {
    glm::dvec3 pos_m;      // Position in meters
    glm::dvec3 vel_m;      // Velocity in m/s
    double mass_kg;        // Mass in kilograms
};
```

### PlanetEditBuffer Struct
*Location: `src/ui/UIManager.h`*

UI buffer for editing planet properties.

```cpp
struct PlanetEditBuffer {
    char name[128];         // Planet name
    float mass;            // Mass in kg
    float density;         // Density in kg/m³
    float radius;          // Radius (calculated)
    glm::vec3 position;    // Position vector
    glm::vec3 velocity;    // Velocity vector
};
```

## 🎮 Input Handling

### Keyboard Controls
Processed in `processInput()` function:
- `GLFW_KEY_W`: Move camera forward
- `GLFW_KEY_S`: Move camera backward
- `GLFW_KEY_A`: Move camera left
- `GLFW_KEY_D`: Move camera right
- `GLFW_KEY_ESCAPE`: Exit application

### Mouse Controls
- **Left Click**: Select planet (if clicked on planet)
- **Right Click + Drag**: Camera rotation
- **Scroll Wheel**: Zoom in/out
- **Hover**: Show planet labels

### Callback Functions
```cpp
void mouseCallback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
```

## 🔧 Constants and Configuration

### Physics Constants
```cpp
// In PhysicsSystem.h
static constexpr double G = 6.67430e-11;        // m³/kg/s²
static constexpr double SOFTEN = 1e3;           // m²

// In main.cpp
constexpr double METERS_PER_WU = 1.0e9;         // m/WU
constexpr double AU = 1.495978707e11;           // m
```

### Rendering Constants
```cpp
// In Planet.h
static constexpr float MIN_PICK_RADIUS = 18.0f; // pixels

// In Planet.cpp
const float scaleFactor = 1e-7f;                // radius scaling
```

### OpenGL Configuration
```cpp
// In main.cpp
glEnable(GL_DEPTH_TEST);
glEnable(GL_BLEND);
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
```

## 🚨 Error Handling

### OpenGL Errors
- Shader compilation errors are logged to console
- OpenGL state errors checked in debug builds
- Resource cleanup in destructors

### File I/O Errors
- Shader file loading failures reported
- Missing shader files cause program termination

### Memory Management
- RAII principles used throughout
- Smart pointers for planet management
- Automatic OpenGL resource cleanup

## 🔍 Debugging Utilities

### Debug Macros
```cpp
#ifdef _DEBUG
#define GL_CHECK(call) \
    call; \
    { GLenum error = glGetError(); \
      if (error != GL_NO_ERROR) \
          std::cerr << "OpenGL error: " << error << std::endl; }
#else
#define GL_CHECK(call) call
#endif
```

### Performance Monitoring
```cpp
// FPS calculation
float fps = 1.0f / deltaTime;
ImGui::Text("FPS: %.1f", fps);
```

---

**This API reference provides complete information for extending and modifying the SolarSystemGL codebase.** 