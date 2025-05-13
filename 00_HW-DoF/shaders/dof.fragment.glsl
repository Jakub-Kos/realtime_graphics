#version 430 core

layout(binding = 0) uniform sampler2D u_scene;
layout(binding = 1) uniform sampler2D u_blur;
layout(binding = 2) uniform sampler2D u_depthMap;

// instead of a fixed focus depth, pass the mouse UV:
layout(location = 0) uniform vec2  u_focusUV;    // normalized [0,1] screen coords
layout(location = 1) uniform float u_focusRange; // half-width in normalized depth

in  vec2 texCoords;
out vec4 fragColor;

void main() {
    // 1) your own fragment’s normalized depth:
    float d = texture(u_depthMap, texCoords).r;

    // 2) sample the mouse’s depth once:
    float focusDepth = texture(u_depthMap, u_focusUV).r;

    // 3) circle-of-confusion in [0,1]:
    float coc = clamp(abs(d - focusDepth) / u_focusRange, 0.0, 1.0);

    // 4) blend:
    vec3 sharp   = texture(u_scene, texCoords).rgb;
    vec3 blurred = texture(u_blur,  texCoords).rgb;
    fragColor    = vec4(mix(sharp, blurred, coc), 1.0);
}
