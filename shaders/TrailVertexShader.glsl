#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 view;
uniform mat4 projection;
uniform int  pointCount;

out float vT;   // 0 at tail, 1 at head, for fragment fade

void main() {
    vT = (pointCount > 1) ? float(gl_VertexID) / float(pointCount - 1) : 1.0;
    gl_Position = projection * view * vec4(aPos, 1.0);
}
