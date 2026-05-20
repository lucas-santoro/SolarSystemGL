#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

#define MAX_PLANETS 32

uniform vec3  planetPositions[MAX_PLANETS];
uniform float planetMasses[MAX_PLANETS];
uniform int   planetCount;

// Distortion knobs (driven by GridSettings on the CPU).
uniform float gridStrength;     // was the magic 0.008
uniform float falloffRadius;    // was the magic 2.0
uniform float maxWellDepth;     // was the magic 38.0

void main() {
    vec3 distortedPos = aPos;
    for (int i = 0; i < planetCount; ++i) {
        vec2  delta      = distortedPos.xz - planetPositions[i].xz;
        float dist2      = max(dot(delta, delta), 0.001);
        float distortion = gridStrength * planetMasses[i]
                         / (1.0 + dist2 / (falloffRadius * falloffRadius));
        distortion       = clamp(distortion, 0.0, maxWellDepth);
        distortedPos.y  -= distortion;
    }

    gl_Position = projection * view * model * vec4(distortedPos, 1.0);
}
