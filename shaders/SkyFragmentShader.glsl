#version 330 core
in  vec2 ndcPos;
out vec4 FragColor;

uniform mat4 invView;
uniform mat4 invProj;

// Cheap hash — repeatable for a given input.
float hash3(vec3 p) {
    return fract(sin(dot(p, vec3(127.1, 311.7, 74.7))) * 43758.5453);
}

void main() {
    // Reconstruct the world-space view direction for this fragment by inverting
    // the projection then the view transform. Stars depend only on direction, so
    // they appear "at infinity" — translating the camera doesn't shift them.
    vec4 clip = vec4(ndcPos, 1.0, 1.0);
    vec4 eye  = invProj * clip;
    eye = vec4(eye.xy, -1.0, 0.0);
    vec3 dir  = normalize(vec3(invView * eye));

    // Quantize direction into cells; bright pixels happen at sparse, high-hash cells.
    vec3 cell = floor(dir * 350.0);
    float r   = hash3(cell);

    vec3 col = vec3(0.0);
    if (r > 0.9965) {
        float brightness = (r - 0.9965) / 0.0035;
        float hue        = hash3(cell + vec3(1.0));
        // Mix between cool blue-white and warm yellow-white stars.
        vec3 tint = mix(vec3(0.7, 0.8, 1.0), vec3(1.0, 0.9, 0.7), hue);
        col = tint * brightness;
    } else if (r > 0.99) {
        // Dimmer second tier — fills out the sky.
        col = vec3((r - 0.99) * 8.0) * 0.3;
    }

    FragColor = vec4(col, 1.0);
}
