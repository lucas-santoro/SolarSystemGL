#version 330 core
layout (location = 0) in vec2 aPos;     // -1..1 fullscreen quad

out vec2 ndcPos;

void main() {
    ndcPos = aPos;
    // z = 1.0 puts us at the far plane; with depth-test disabled the order matters,
    // but we render the sky first anyway so the value isn't load-bearing.
    gl_Position = vec4(aPos, 1.0, 1.0);
}
