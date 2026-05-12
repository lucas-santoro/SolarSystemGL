#version 330 core
in  vec2  vUV;
out vec4  FragColor;

uniform vec3 color;

void main() {
    float r = length(vUV);
    if (r > 1.0) discard;

    // Soft quadratic falloff. With additive blending (GL_ONE, GL_ONE) this
    // accumulates with whatever's behind it; intensity is the only knob.
    float intensity = pow(1.0 - r, 2.5) * 1.4;
    FragColor = vec4(color * intensity, 1.0);
}
