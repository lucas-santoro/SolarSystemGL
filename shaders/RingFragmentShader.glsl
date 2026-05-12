#version 330 core
in  vec3 vLocal;
out vec4 FragColor;

uniform vec3 ringColor;

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

    FragColor = vec4(ringColor * band, alpha * 0.85);
}
