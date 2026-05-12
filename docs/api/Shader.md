# `Shader`

`src/core/Shader.h`

Thin wrapper around a compiled-and-linked OpenGL program. Loads vertex + fragment GLSL from disk, exposes typed uniform setters.

## Definition

```cpp
class Shader {
public:
    Shader(const std::string& vertexPath, const std::string& fragmentPath);
    ~Shader();

    void         use();
    unsigned int getID() const;

    void setMat4(const std::string& name, const glm::mat4& mat);
    void setVec3(const std::string& name, const glm::vec3& vec);
    void setVec3Array(const std::string& name, const std::vector<glm::vec3>& values);
    void setFloat(const std::string& name, float value);
    void setFloatArray(const std::string& name, const std::vector<float>& values);
    void setInt(const std::string& name, int value);

private:
    unsigned int ID;
    bool        compileShader(const std::string& vertexCode, const std::string& fragmentCode);
    std::string readFile(const std::string& filePath);
};
```

## Lifecycle

The constructor:
1. Reads both files via `readFile`.
2. Compiles vertex + fragment shaders, links the program.
3. Stores the program ID in `ID`.

Errors at any stage print to `std::cerr` and **the constructor returns anyway**, leaving `ID` in whatever state `glCreateProgram()` returned (usually 0). `use()` will then bind program 0 which falls through to the OpenGL fixed-function pipeline — the famous "everything is white" symptom.

`readFile` fails loud now (added in this iteration): missing files print `Failed to open shader file: <path>`. Pre-fix, missing files returned an empty string and the compile error message was the only signal.

A P1 item proposes throwing on shader compile failure rather than continuing silently.

## Uniform setters

All setters call `glGetUniformLocation(ID, name.c_str())` on **every** invocation. For 9 bodies × 3 uniforms per frame at 60 FPS that's ~1 600 lookups per second — negligible at this scale, wasteful in principle. Caching the location at construction is a P1 item.

The shader does not validate that the uniform exists — `glGetUniformLocation` returns `-1` for missing names, and `glUniform*` with `-1` is a silent no-op. That's defensive when you're iterating shader sources, but it can hide typos.

## Files

The current shader sources are:

| File | Stage | Used by |
|---|---|---|
| `shaders/VertexShader.glsl` | vertex | bodies |
| `shaders/FragmentShader.glsl` | fragment | bodies |
| `shaders/GridVertexShader.glsl` | vertex | grid |
| `shaders/GridFragmentShader.glsl` | fragment | grid |

Build copies them next to the executable via a CMake `POST_BUILD` step using `$<TARGET_FILE_DIR:SolarSystemGL>`, so the exe finds them regardless of CWD.

## Uniforms in current use

**Bodies pipeline:**
- `mat4 model, view, projection` — set per frame.
- `vec3 planetColor` — diffuse base color, set per body.
- `float emissive` — 0.0 (lit) or 1.0 (star), set per body.
- `vec3 lightPos` — Sun's render position, set once per frame.
- `vec3 viewPos` — camera position, set once per frame (for specular).

**Grid pipeline:**
- `mat4 model, view, projection` — set per frame.
- `int planetCount` — number of bodies up to 10.
- `vec3 planetPositions[10]` — render-space positions.
- `float planetMasses[10]` — Earth-mass-normalized.
