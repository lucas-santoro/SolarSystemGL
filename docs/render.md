# Rendering

OpenGL 3.3 core profile. Two draw passes per frame: bodies, then the grid. ImGui draws after both.

## Frame structure

```
glClear(COLOR | DEPTH)

shader.use()                          ← bodies pipeline
  set view, projection, model = I
  set lightPos = bodies[0].renderPosition()
  set viewPos  = camera.getPosition()
  for body in bodies:
      body.render(shader, hovered)    ← sets model, planetColor, emissive; mesh.draw()

gridShader.use()                      ← grid pipeline
  set view, projection, model = I
  grid.draw(gridShader, bodies)       ← uploads planetPositions[], planetMasses[]

imgui pass                            ← always on top
```

## Bodies pipeline

**Vertex shader** (`shaders/VertexShader.glsl`):
```glsl
in vec3 aPos;
out vec3 FragPos, Normal;

void main() {
    vec4 wp = model * vec4(aPos, 1.0);
    FragPos = wp.xyz;
    Normal  = normalize(aPos);  // valid because model has uniform scale + translation only
    gl_Position = projection * view * wp;
}
```

The normal trick: vertices on a sphere centered at the origin satisfy `aPos = radius · n̂`, so `n̂ = normalize(aPos)`. Since the model matrix applies only translation and uniform scale, the direction is preserved into world space. No `normalMatrix` needed; no normal attribute needed.

**Fragment shader** (`shaders/FragmentShader.glsl`):
```glsl
if (emissive > 0.5) {
    FragColor = vec4(planetColor * 1.6, 1.0);   // stars
    return;
}

vec3 L = normalize(lightPos - FragPos);
vec3 V = normalize(viewPos  - FragPos);
vec3 H = normalize(L + V);

float diffuse  = max(dot(Normal, L), 0.0);
float specular = pow(max(dot(Normal, H), 0.0), 32.0) * 0.15 * diffuse;
float ambient  = 0.05;

FragColor = vec4(planetColor * (ambient + 0.95 * diffuse) + vec3(specular), 1.0);
```

- **Lambert diffuse** with `lightPos = bodies[0].renderPosition()` (Sun is always body 0).
- **Blinn-Phong specular** is subtle (0.15 factor) — gives a hint of sheen near the terminator without making rocky planets look wet.
- **Ambient floor** of 5 % so the night side isn't pitch-black.
- **Emissive bypass**: stars skip all lighting and return `color * 1.6`, brighter than 1.0 in linear space (gets clamped by the LDR framebuffer for now; HDR + bloom is a P2 item).

## Grid pipeline

The grid is a flat XZ line mesh built in `Grid::setupGrid`. Each frame, `Grid::draw` uploads up to 10 body positions and masses (normalized by Earth mass) as uniform arrays. The vertex shader sums per-body gravitational depressions:

```glsl
for (i = 0; i < planetCount; ++i) {
    vec2 delta = aPos.xz - planetPositions[i].xz;
    float dist2 = max(dot(delta, delta), 0.001);
    float depth = 0.008 * planetMasses[i] / (1.0 + dist2 / 4.0);
    distortedPos.y -= clamp(depth, 0.0, 38.0);
}
```

It's a visual approximation — not a physically derived metric — but cheap and reads well as "spacetime curvature".

The 10-body cap is enforced both client-side (`Grid::draw`'s `std::min(...,10)`) and shader-side (`GridVertexShader.glsl` `MAX_PLANETS = 10`). Increasing it requires editing both.

## State management

Each frame, before drawing planets:
- `glEnable(GL_DEPTH_TEST)` was set once at startup; never disabled.
- `glPolygonMode(GL_FRONT_AND_BACK, GL_FILL)` is set inside `CelestialBody::render`. Redundant after the first body, harmless.
- `glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)` was set once at startup. The grid fragment shader emits `vec4(1, 1, 1, 0.2)` and relies on this.

There are no framebuffer objects yet. HDR, bloom, and post-processing are P2.

## Mesh

`PlanetMesh::rebuild(float radius)` generates a subdivided icosahedron (3 subdivisions by default, ~1280 triangles). The base 12 vertices are normalized and scaled to radius; subdivision uses an edge-midpoint cache (`std::map<pair, index>`) to avoid duplicating vertices. Upload via `glBufferData(GL_STATIC_DRAW)`. The same VAO is re-used across `rebuild` calls; only `glBufferData` re-uploads.

## Why no textures yet

Texture mapping is in P1. It needs three things this session didn't include:
1. UV coords on the mesh (extension of `PlanetMesh::rebuild` to emit a 5-float vertex format).
2. An image loader (typically `stb_image.h`, single-header, public domain).
3. Actual texture assets (NASA imagery is public-domain).

Once those land, the fragment shader gets a `sampler2D` and the lit body color becomes `texture(diffuseTex, UV) * lightingTerm`.
