#version 330 core

in  float vWellDepth;
in  float vDistanceFromCam;
in  float vIsMajorLine;
in  vec3  vPerBodyColor;

out vec4 FragColor;

uniform vec3  baseColor;
uniform vec3  wellColor;
uniform float opacity;
uniform float majorLineBoost;
uniform float maxWellDepth;
uniform float distanceFadeStart;
uniform float distanceFadeEnd;
uniform bool  perBodyTint;
uniform float singularityDarken;

void main()
{
    // Depth gradient: lerp base → well color by normalized well depth.
    float depthT = clamp(vWellDepth / max(maxWellDepth, 0.01), 0.0, 1.0);
    vec3  color  = mix(baseColor, wellColor, depthT);

    // Per-body color tint scales with depth so flat regions stay neutral.
    if (perBodyTint) {
        color = mix(color, vPerBodyColor, 0.6 * depthT);
    }

    // Singularity darkening — applies to the deepest fraction of the well,
    // mixing toward pure black. Scales by `singularityDarken` so users can
    // dial it off (0.0) or push past the natural max (1.5).
    float singularityT = smoothstep(maxWellDepth * 0.5, maxWellDepth, vWellDepth);
    float darken       = clamp(singularityDarken * singularityT, 0.0, 1.0);
    color              = mix(color, vec3(0.0), darken);

    // Major lines are brighter (alpha boost). Minor lines stay at 1x.
    float majorBoost = (vIsMajorLine > 0.5) ? majorLineBoost : 1.0;

    // Distance fade: opaque near the camera, zero past distanceFadeEnd.
    float distanceFade = 1.0 - smoothstep(distanceFadeStart, distanceFadeEnd, vDistanceFromCam);

    FragColor = vec4(color, opacity * majorBoost * distanceFade);
}
