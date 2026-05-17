#version 330 core

in  vec2 vUV;
out vec4 FragColor;

uniform sampler2D source;
uniform bool      horizontal;

// 9-tap Gaussian weights (sigma ~ 2.0), normalized so the centre + 4 mirrored
// taps on each side sum to 1.0.
const float kWeights[5] = float[](
    0.227027,
    0.1945946,
    0.1216216,
    0.054054,
    0.016216
);

void main() {
    vec2 texelSize = 1.0 / vec2(textureSize(source, 0));
    vec3 result    = texture(source, vUV).rgb * kWeights[0];

    if (horizontal) {
        for (int i = 1; i < 5; ++i) {
            vec2 offset = vec2(texelSize.x * float(i), 0.0);
            result += texture(source, vUV + offset).rgb * kWeights[i];
            result += texture(source, vUV - offset).rgb * kWeights[i];
        }
    } else {
        for (int i = 1; i < 5; ++i) {
            vec2 offset = vec2(0.0, texelSize.y * float(i));
            result += texture(source, vUV + offset).rgb * kWeights[i];
            result += texture(source, vUV - offset).rgb * kWeights[i];
        }
    }

    FragColor = vec4(result, 1.0);
}
