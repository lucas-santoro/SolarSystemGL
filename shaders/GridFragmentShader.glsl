#version 330 core
out vec4 FragColor;

uniform vec3  baseColor;
uniform float opacity;

void main()
{
    FragColor = vec4(baseColor, opacity);
}
