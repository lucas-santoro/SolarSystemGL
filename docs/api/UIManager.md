# `UIManager`

`src/ui/UIManager.h`

Owns the ImGui panels, the picking pass, and the per-body edit buffer. Stateful across frames (`selectedPlanetIndex`, `smoothedFps`).

## Definition

```cpp
class UIManager {
public:
    void render(Window& window, Camera& camera, float deltaTime,
                std::vector<CelestialBody>& bodies, Grid& grid, PhysicsSystem& physics);

    bool isRightMousePressed(GLFWwindow* window);
    bool isHovered(size_t i) const;

private:
    int   selectedPlanetIndex = -1;
    int   hoveredIndex        = -1;
    int   lastSelectedIndex   = -1;
    bool  isMouseMoving       = false;
    float smoothedFps         = 60.0f;

    struct PlanetEditBuffer {
        char       name[128];
        float      mass;
        float      density;
        float      radius;
        glm::vec3  position;
        glm::vec3  velocity;
    } editBuffer;

    void renderPlanetPopup(...);
    void renderPlanetInfo(CelestialBody& body, Camera& camera);
    void renderMainPanel(float deltaTime, std::vector<CelestialBody>& bodies,
                        Grid& grid, PhysicsSystem& physics);
    void renderNavbar(const std::vector<CelestialBody>& bodies);
};
```

## Frame flow

`render()` is called once per render frame after `physics.update()` and the body/grid draws. Order:

1. **`renderNavbar`** — top menu bar with the Planets dropdown.
2. **`renderPlanetPopup`** — picks hover/selection, draws the floating name label.
3. **`renderMainPanel`** — FPS, paused, time scale, Add Planet.
4. If something is selected, **`renderPlanetInfo`** — edit panel.

## Picking

`renderPlanetPopup` ray-casts the mouse against every body each frame:

```cpp
glm::vec3 rayDir  = camera.getRayFromMouse(mouseX, mouseY, ...);
glm::vec3 rayOrig = camera.getPosition();
for (size_t i = 0; i < bodies.size(); ++i) {
    if (bodies[i].intersectsRay(rayOrig, rayDir)) {
        // keep the closest hit
    }
}
```

`intersectsRay` uses a minimum pick radius of 18 WU so visually-tiny bodies (Mercury) are still clickable.

If the mouse is over an ImGui window (`ImGui::GetIO().WantCaptureMouse`), picking is skipped — clicks inside the UI don't bleed into the world.

## Edit buffer

The user's typed edits go into `editBuffer`, **not** directly into the body. The buffer is repopulated whenever the selection changes:

```cpp
if (selectedPlanetIndex != lastSelectedIndex) {
    editBuffer.mass     = static_cast<float>(body.mass_kg);
    editBuffer.density  = body.density;
    editBuffer.position = vec3(body.pos_m / METERS_PER_WU);
    editBuffer.velocity = vec3(body.vel_m / 1000.0);   // m/s → km/s
    ...
}
```

Clicking **Apply Changes** copies the buffer into the body:

```cpp
body.mass_kg  = static_cast<double>(editBuffer.mass);
body.density  = editBuffer.density;
body.pos_m    = dvec3(editBuffer.position) * METERS_PER_WU;
body.vel_m    = dvec3(editBuffer.velocity) * 1000.0;
body.recalculateGeometry();
```

The buffer pattern means in-flight edits don't affect the simulation. Reset reverts the buffer to current body state without applying.

See [ADR 0003](../adr/0003-apply-button-vs-live-edit.md) for the rationale of keeping Apply rather than live-editing.

## FPS smoothing

`smoothedFps` is an exponential moving average (α = 0.1):

```cpp
const float instantFps = 1.0f / std::max(deltaTime, 1e-6f);
smoothedFps = smoothedFps * 0.9f + instantFps * 0.1f;
```

This roughly averages over the last 10 frames, eliminating the per-frame jitter of `1/dt`.

## Add Planet

`renderMainPanel`'s button:

```cpp
const double a     = 1.5 * AU;
const double angle = static_cast<double>(bodies.size()) * 0.7;
const double v     = std::sqrt(PhysicsSystem::G * 1.989e30 / a);

CelestialBody nb;
nb.pos_m   = dvec3(a * cos(angle), 0.0, a * sin(angle));
nb.vel_m   = dvec3(-v * sin(angle), 0.0, v * cos(angle));
nb.mass_kg = 1.0e24;
...
nb.recalculateGeometry();
bodies.push_back(std::move(nb));
selectedPlanetIndex = bodies.size() - 1;
```

The angle stagger (`bodies.size() * 0.7` radians ≈ 40° per add) keeps successive bodies from stacking. The circular orbital velocity prevents the new body from immediately falling into the Sun. Auto-selecting it gives the user immediate visual feedback in the Planet Info panel.

## Notes

- `editBuffer.radius` is in the struct but unused — radius is computed by `recalculateGeometry`. Tracked for cleanup.
- `isMouseMoving` is dead state. Same.
- `renderMainPanel` takes `Grid&` but doesn't use it currently — kept in the signature for future grid-tuning controls.
