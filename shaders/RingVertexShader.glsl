#version 330 core
layout (location = 0) in vec3 aPos;   // unit ring vertex on the XZ plane (radii 1.0 / 1.5)

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 vLocal;

void main() {
    vLocal = aPos;   // fragment uses this to compute radius for banding & alpha
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
