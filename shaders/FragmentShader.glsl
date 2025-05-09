#version 330 core
out vec4 FragColor;

uniform vec3 planetColor;

void main()
{
    FragColor = vec4(planetColor, 1.0);
}
