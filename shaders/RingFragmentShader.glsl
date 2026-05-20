#version 330 core
in  vec3 vLocal;
in  vec3 vWorldPos;

out vec4 FragColor;

uniform vec3  ringColor;
uniform vec3  bodyCenter;   // Saturn's centre in world units
uniform float bodyRadius;   // Saturn's display radius in world units
uniform vec3  lightPos;     // dominant light (Sun)

void main() {
    // Mesh has inner radius 1.0 and outer radius 1.5.
    float r = length(vLocal.xz);
    if (r < 1.0 || r > 1.5) discard;

    // Normalize r to 0..1 across the ring width.
    float t = (r - 1.0) / 0.5;

    // Banding pattern — alternating dense and sparse particles.
    float band = 0.6 + 0.4 * sin(t * 28.0);

    // Soft fade at both edges so the ring blends into space.
    float alpha = smoothstep(0.0, 0.08, t) * (1.0 - smoothstep(0.88, 1.0, t));

    // Body shadow: cast a ray from this ring point toward the light. If it
    // pierces Saturn's sphere before reaching the light, the ring is in
    // Saturn's shadow.
    vec3  toLight = lightPos - vWorldPos;
    vec3  oc      = vWorldPos - bodyCenter;
    float a       = dot(toLight, toLight);
    float b       = 2.0 * dot(oc, toLight);
    float c       = dot(oc, oc) - bodyRadius * bodyRadius;
    float disc    = b * b - 4.0 * a * c;
    float shadow  = 1.0;
    if (disc > 0.0) {
        float sd = sqrt(disc);
        float t0 = (-b - sd) / (2.0 * a);
        float t1 = (-b + sd) / (2.0 * a);
        bool  hits = (t0 > 0.0 && t0 < 1.0) || (t1 > 0.0 && t1 < 1.0);
        if (hits) shadow = 0.30;
    }

    FragColor = vec4(ringColor * band * shadow, alpha * 0.85);
}
