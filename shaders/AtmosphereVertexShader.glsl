#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 vWorldPos;
out vec3 vNormal;

void main() {
    vec4 worldPos = model * vec4(aPos, 1.0);
    vWorldPos     = worldPos.xyz;

    // Mesh vertices are normalize(unit_pos) * radius, so the surface normal
    // in model space is `normalize(aPos)`. The model matrix carries only
    // uniform scale + translation (no rotation), so this direction is
    // unchanged in world space.
    vNormal       = normalize(aPos);

    gl_Position = projection * view * worldPos;
}
