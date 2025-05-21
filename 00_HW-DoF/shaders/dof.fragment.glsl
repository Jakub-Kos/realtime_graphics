#version 430 core

layout(binding = 0) uniform sampler2D u_scene;      // sharp lit scene
layout(binding = 1) uniform sampler2D u_1blur;       // light blurred result
layout(binding = 2) uniform sampler2D u_2blur;       // heavy blurred result
layout(binding = 3) uniform sampler2D u_depthMap;

// instead of a fixed focus depth, pass the mouse UV:
layout(location = 0) uniform vec2  u_focusUV;    // normalized [0,1] screen coords
layout(location = 1) uniform float u_focusRange; // half-width in normalized depth

in vec2 texCoords;
out vec4 fragColor;

void main() {
    // 1) your own fragment’s normalized depth:
    float d = texture(u_depthMap, texCoords).r;

    // 2) sample the mouse’s depth once:
    float focusDepth = texture(u_depthMap, u_focusUV).r;
    float diff = abs(d - focusDepth);

    // 3) circle-of-confusion in [0,1]:
    float coc = clamp(abs(d - focusDepth) / u_focusRange, 0.0, 1.0);

    // 4) fetch all three
    vec3 sharp = texture(u_scene, texCoords).rgb;
    vec3 blur1 = texture(u_1blur, texCoords).rgb;
    vec3 blur2 = texture(u_2blur, texCoords).rgb;

    if (diff < u_focusRange) {
        // inside focus band: show sharp
        fragColor = vec4(mix(sharp, blur1, coc), 1.0);
    } else {
        // outside focus: normalize how far past the band you are
        // here we assume depth in [0,1], so max distance is (1 - u_focusRange)
        float t = 1 / (diff - u_focusRange);
        t = clamp(t, 0.0, 1.0);
        //t = smoothstep(0.0, 1.0, t);

        // blend only the two blurs
        t = pow(t, 0.5);
        vec3 blurMix = mix(blur2, blur1, t);
        //fragColor    = vec4(blurMix, 1.0);
        //fragColor = vec4(1/diff, 1/diff, 1/diff, 1.0);
        fragColor = vec4(t,t,t,1.0);
    }
    //fragColor = vec4(mix(sharp, blurMix, coc), 1.0);
}