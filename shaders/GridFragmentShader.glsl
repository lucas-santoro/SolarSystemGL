#version 330 core

in  float vWellDepth;
in  float vDistanceFromCam;
in  float vIsMajorLine;

out vec4 FragColor;

uniform vec3  baseColor;
uniform vec3  wellColor;
uniform float opacity;
uniform float majorLineBoost;
uniform float maxWellDepth;
uniform float distanceFadeStart;
uniform float distanceFadeEnd;

void main()
{
    // Depth gradient: lerp base → well color by normalized well depth.
    float depthT = clamp(vWellDepth / max(maxWellDepth, 0.01), 0.0, 1.0);
    vec3  color  = mix(baseColor, wellColor, depthT);

    // Major lines are brighter (alpha boost). Minor lines stay at 1x.
    float majorBoost = (vIsMajorLine > 0.5) ? majorLineBoost : 1.0;

    // Distance fade: opaque near the camera, zero past distanceFadeEnd.
    float distanceFade = 1.0 - smoothstep(distanceFadeStart, distanceFadeEnd, vDistanceFromCam);

    FragColor = vec4(color, opacity * majorBoost * distanceFade);
}
