#version 330 core
layout (location = 0) in vec3 aPos;   // unit ring vertex on the XZ plane (radii 1.0 / 1.5)

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 vLocal;     // fragment uses this to compute radius for banding & alpha
out vec3 vWorldPos;  // fragment uses this for body-shadow ray casting

void main() {
    vLocal      = aPos;
    vWorldPos   = (model * vec4(aPos, 1.0)).xyz;
    gl_Position = projection * view * vec4(vWorldPos, 1.0);
}
