#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 FragPos;     // world-space position
out vec3 Normal;      // world-space normal

void main() {
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos       = worldPos.xyz;

    // Mesh vertices are normalize(unit_pos) * radius, so the surface normal
    // in model-space is just normalize(aPos). Because the model matrix only
    // applies uniform scale + translation (no rotation), this direction is
    // unchanged in world-space.
    Normal = normalize(aPos);

    gl_Position = projection * view * worldPos;
}
