#version 330 core
in vec3 FragPos;
in vec3 Normal;

out vec4 FragColor;

uniform vec3  planetColor;
uniform float emissive;   // 0.0 = receives lighting, 1.0 = self-illuminated (Sun)
uniform vec3  lightPos;   // world-space position of the dominant light (Sun)
uniform vec3  viewPos;    // world-space camera position (for specular)

// Ring-shadow uniforms (only consulted when hasRingShadow is true).
uniform bool  hasRingShadow;
uniform vec3  ringCenter;      // world-space centre of the ring disk (also the body)
uniform float ringInnerRadius; // world units
uniform float ringOuterRadius; // world units

void main()
{
    if (emissive > 0.5) {
        // Stars: emit their own color, bright-clamped.
        FragColor = vec4(planetColor * 1.6, 1.0);
        return;
    }

    vec3 N = normalize(Normal);
    vec3 L = normalize(lightPos - FragPos);

    // Diffuse (Lambert)
    float NdotL  = max(dot(N, L), 0.0);

    // Ambient floor so the dark side isn't pitch-black
    float ambient = 0.05;

    // Subtle specular highlight (Blinn-Phong) — gives planets a faint terminator sheen
    vec3  V          = normalize(viewPos - FragPos);
    vec3  H          = normalize(L + V);
    float NdotH      = max(dot(N, H), 0.0);
    float specular   = pow(NdotH, 32.0) * 0.15 * NdotL;

    vec3 lit = planetColor * (ambient + NdotL * 0.95) + vec3(specular);

    // Ring shadow — intersect the fragment-to-light ray with the ring's
    // XZ plane and darken the body where the ray passes through the disk.
    if (hasRingShadow) {
        vec3  toLight = lightPos - FragPos;
        if (abs(toLight.y) > 1e-5) {
            float t = (ringCenter.y - FragPos.y) / toLight.y;
            if (t > 0.0 && t < 1.0) {
                vec2  hitXZ = FragPos.xz + t * toLight.xz;
                float r     = length(hitXZ - ringCenter.xz);
                // Soft edges: penumbra of ~5% the ring width on each side.
                float edge  = (ringOuterRadius - ringInnerRadius) * 0.05;
                float shadow = smoothstep(ringInnerRadius - edge, ringInnerRadius + edge, r)
                             * (1.0 - smoothstep(ringOuterRadius - edge, ringOuterRadius + edge, r));
                lit *= mix(1.0, 0.35, shadow);
            }
        }
    }

    FragColor = vec4(lit, 1.0);
}
