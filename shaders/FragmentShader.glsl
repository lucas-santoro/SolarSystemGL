#version 330 core
in vec3 FragPos;
in vec3 Normal;

out vec4 FragColor;

uniform vec3  planetColor;
uniform float emissive;   // 0.0 = receives lighting, 1.0 = self-illuminated (Sun)
uniform vec3  lightPos;   // world-space position of the dominant light (Sun)
uniform vec3  viewPos;    // world-space camera position (for specular)

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
    FragColor = vec4(lit, 1.0);
}
