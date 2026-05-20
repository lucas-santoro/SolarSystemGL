#version 330 core
layout (location = 0) in vec3  aPos;
layout (location = 1) in float aLineIndex;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

#define MAX_PLANETS 32

uniform vec3  planetPositions[MAX_PLANETS];
uniform float planetMasses[MAX_PLANETS];
uniform int   planetCount;

uniform float gridStrength;     // GridSettings.distortionStrength
uniform float falloffRadius;    // GridSettings.falloffRadius
uniform float maxWellDepth;     // GridSettings.maxWellDepth

uniform vec3  cameraPos;
uniform int   majorLineInterval;

out float vWellDepth;        // positive when the vertex is in a well
out float vDistanceFromCam;  // world-space distance to the camera
out float vIsMajorLine;      // 1.0 on major lines, 0.0 otherwise

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

    vWellDepth = -distortedPos.y;

    vec3 worldPos     = (model * vec4(distortedPos, 1.0)).xyz;
    vDistanceFromCam  = length(cameraPos - worldPos);

    int  interval     = (majorLineInterval > 0) ? majorLineInterval : 1;
    float remainder   = mod(aLineIndex, float(interval));
    vIsMajorLine      = (remainder < 0.5) ? 1.0 : 0.0;

    gl_Position = projection * view * vec4(worldPos, 1.0);
}
