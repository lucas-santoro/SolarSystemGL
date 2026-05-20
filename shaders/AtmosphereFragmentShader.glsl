#version 330 core

in  vec3 vWorldPos;
in  vec3 vNormal;
out vec4 FragColor;

uniform vec3  atmosphereColor;
uniform vec3  viewPos;
uniform vec3  lightPos;
uniform float density;

void main() {
    vec3 V = normalize(viewPos  - vWorldPos);
    vec3 L = normalize(lightPos - vWorldPos);
    vec3 N = vNormal;

    // Fresnel-style rim: brightest at the silhouette, falls off toward the
    // dead-on direction. Approximates the longer light path through the
    // atmosphere near the limb.
    float rim   = pow(1.0 - clamp(dot(N, V), 0.0, 1.0), 2.5);

    // Day-side modulation — the night side keeps a thin glow so the limb
    // remains visible everywhere, but the lit half gets the most colour.
    float ndotl = max(dot(N, L), 0.0);
    float dayMix = 0.25 + 0.75 * ndotl;

    float intensity = rim * dayMix * density;

    // Alpha-blended halo on top of the body. The intensity drives both the
    // colour brightness and the per-pixel opacity, so the shell sits tight
    // against the limb and disappears toward the dead-on centre.
    FragColor = vec4(atmosphereColor * intensity, intensity * 0.85);
}
