#version 330 core

in  vec2 vUV;
out vec4 FragColor;

uniform sampler2D scene;
uniform sampler2D bloom;
uniform float     bloomIntensity;

void main() {
    vec3 sceneColor = texture(scene, vUV).rgb;
    vec3 bloomColor = texture(bloom, vUV).rgb * bloomIntensity;
    FragColor       = vec4(sceneColor + bloomColor, 1.0);
}
