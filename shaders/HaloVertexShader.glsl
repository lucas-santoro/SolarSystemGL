#version 330 core
layout (location = 0) in vec2 aQuadPos;  // unit quad corners in [-1, 1]

uniform mat4  view;
uniform mat4  projection;
uniform vec3  center;   // world-space position of the emissive body
uniform float size;     // world-space halo radius

out vec2 vUV;           // -1..1, used for radial fade in fragment

void main() {
    // Reconstruct camera-aligned right/up axes from the view matrix so the
    // quad faces the camera regardless of orientation (cheap billboarding).
    vec3 right = vec3(view[0][0], view[1][0], view[2][0]);
    vec3 up    = vec3(view[0][1], view[1][1], view[2][1]);

    vec3 worldPos = center + (right * aQuadPos.x + up * aQuadPos.y) * size;

    vUV = aQuadPos;
    gl_Position = projection * view * vec4(worldPos, 1.0);
}
