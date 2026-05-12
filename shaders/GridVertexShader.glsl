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
        float distortion = 0.008 * planetMasses[i] / (1.0 + dist2 / (2.0 * 2.0));
        distortion = clamp(distortion, 0.0, 38.0);
        distortedPos.y -= distortion;
    }

    gl_Position = projection * view * model * vec4(distortedPos, 1.0);
}
