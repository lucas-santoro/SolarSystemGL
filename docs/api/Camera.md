# `Camera`

`src/core/Camera.h`

Two-mode camera (FREE / ORBITAL) with smooth-move helper, ray-cast for picking, and world-to-screen projection for hover labels.

## Modes

```cpp
enum class CameraMode { FREE, ORBITAL };
```

- **FREE** — WASD movement, right-drag looks around. The `position`, `front`, `yaw`, `pitch` are all live.
- **ORBITAL** — locked to a target body. Orbit angle controlled by right-drag (yaw/pitch); distance by scroll. `position` is computed each frame from spherical coords around the target.

`setMode(CameraMode)` switches modes. Returning to FREE clears the orbital target index.

## Orbital target — by index, not pointer

```cpp
int       orbitalTargetIndex = -1;
glm::vec3 orbitalTargetPos{ 0.0f };
```

The orbital target is a **stable integer index** into `std::vector<CelestialBody> bodies`. Each frame, `main.cpp` reads `bodies[orbitalTargetIndex].renderPosition()` and pushes it via `setOrbitalTargetPos`. The view matrix then orbits around that cached position.

The previous design used `const glm::vec3*` pointing inside a `Planet`. That worked until `bodies.push_back(...)` triggered a vector reallocation — the pointer dangled and the camera crashed or jumped to garbage. Indices are immune to reallocation. See [ADR 0002](../adr/0002-orbital-target-by-index.md).

`bodies.reserve(16)` at startup also keeps actual addresses stable for the first 16 bodies — index + reserve is defense in depth.

## Smooth move

```cpp
void startSmoothMove(const glm::vec3& destination, float distance = 60.0f);
void update(float dt);  // tick the smooth move toward the destination
```

When the user clicks on a body in FREE mode, `UIManager::renderPlanetPopup` calls:

```cpp
camera.startSmoothMove(body.renderPosition(), max(50.0f, body.radius * 4.0f));
```

`update(dt)` then walks `position` toward `destination` at `travelSpeed = 3000.0f` units/s, snapping when within 0.1 units. In ORBITAL mode, smooth-move is bypassed — orbital camera is positionally determined by `(target, yaw, pitch, distance)`.

## Picking

```cpp
glm::vec3 getRayFromMouse(double mouseX, double mouseY,
                          int screenWidth, int screenHeight,
                          const glm::mat4& view, const glm::mat4& projection);
```

Standard NDC → eye → world ray reconstruction. Returns the normalized direction. `UIManager::renderPlanetPopup` uses this and `camera.getPosition()` as the ray origin, then iterates `bodies` calling `intersectsRay`.

## World → screen

```cpp
glm::vec2 worldToScreen(const glm::vec3& worldPos,
                        const glm::mat4& view, const glm::mat4& projection,
                        int screenWidth, int screenHeight);
```

Returns pixel coordinates of `worldPos`, or `vec2(-1, -1)` if it would project behind the camera. Used to place the hover label slightly above the picked body.

## Notes

- `getViewMatrix()` is not `const` — it writes to `this->position` in ORBITAL mode (computed from yaw/pitch/distance). Don't lock to `const Camera&` if you need the view matrix.
- `processMouseScroll` does different things per mode: zoom in ORBITAL, change `speed` in FREE.
- The orbital camera computes spherical coordinates with the convention `y = sin(pitch)`. Pitch is clamped to ±89° to avoid gimbal singularity.
