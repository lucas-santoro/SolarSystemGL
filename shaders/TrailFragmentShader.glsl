#version 330 core
in  float vT;
out vec4  FragColor;

uniform vec3 trailColor;

void main() {
    // Tail fades to ~0.05 alpha, head reaches ~0.7. Quadratic for softer falloff.
    float alpha = 0.05 + 0.65 * vT * vT;
    FragColor = vec4(trailColor, alpha);
}
