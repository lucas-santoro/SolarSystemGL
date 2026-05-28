#version 330 core
layout (location = 0) in vec3  aPos;
layout (location = 1) in float aLineIndex;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

#define MAX_PLANETS 32

uniform vec3  planetPositions[MAX_PLANETS];
uniform float planetMasses[MAX_PLANETS];
uniform vec3  planetColors[MAX_PLANETS];
uniform int   planetCount;

uniform float gridStrength;     // GridSettings.distortionStrength
uniform float falloffRadius;    // GridSettings.falloffRadius
uniform float maxWellDepth;     // GridSettings.maxWellDepth
uniform bool  useSchwarzschild; // GridSettings.useSchwarzschild

uniform vec3  cameraPos;
uniform int   majorLineInterval;

out float vWellDepth;        // positive when the vertex is in a well
out float vDistanceFromCam;  // world-space distance to the camera
out float vIsMajorLine;      // 1.0 on major lines, 0.0 otherwise
out vec3  vPerBodyColor;     // weighted blend of contributing body colors

void main() {
    vec3  distortedPos    = aPos;
    vec3  weightedColor   = vec3(0.0);
    float totalDepth      = 0.0;

    for (int i = 0; i < planetCount; ++i) {
        vec2  delta = distortedPos.xz - planetPositions[i].xz;
        float dist2 = max(dot(delta, delta), 0.001);

        float depth;
        if (useSchwarzschild) {
            // Schwarzschild-inspired: depth ~ M / r. Softening keeps the
            // singularity finite (r is in world units; the 0.5 softening
            // matches the renderer's typical body display radius).
            float r = sqrt(dist2);
            depth = gridStrength * planetMasses[i] / max(r, 0.5);
            depth = clamp(depth, 0.0, maxWellDepth);
        } else {
            // Smooth "rubber-sheet" well. The peak depth saturates with mass
            // through 1 - exp(-k·m), so it approaches maxWellDepth without a
            // hard clamp — the bottom is a rounded Lorentzian apex, never a
            // flat plateau. The well also widens with that depth, so massive
            // bodies carve a broad, gentle bowl that crosses many grid lines
            // (instead of a narrow, faceted spike).
            float m      = planetMasses[i];
            float peak   = maxWellDepth * (1.0 - exp(-gridStrength * m));
            float radius = falloffRadius + peak * 2.0;
            depth        = peak / (1.0 + dist2 / (radius * radius));
        }

        distortedPos.y -= depth;

        weightedColor += planetColors[i] * depth;
        totalDepth    += depth;
    }

    vWellDepth     = -distortedPos.y;
    vPerBodyColor  = (totalDepth > 0.0001) ? (weightedColor / totalDepth) : vec3(0.0);

    vec3 worldPos    = (model * vec4(distortedPos, 1.0)).xyz;
    vDistanceFromCam = length(cameraPos - worldPos);

    int   interval  = (majorLineInterval > 0) ? majorLineInterval : 1;
    float remainder = mod(aLineIndex, float(interval));
    vIsMajorLine    = (remainder < 0.5) ? 1.0 : 0.0;

    gl_Position = projection * view * vec4(worldPos, 1.0);
}
