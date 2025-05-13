#version 430 core

layout(binding = 0) uniform sampler2D u_scene;    // sharp lit scene
layout(binding = 1) uniform sampler2D u_blur;     // blurred result
layout(binding = 2) uniform sampler2D u_depthMap;

layout(location = 0) uniform float u_focusDist;   // focal distance
layout(location = 1) uniform float u_focusRange;  // half-width of sharp zone

in vec2 texCoords;
out vec4 fragColor;


uniform float u_nearPlane;  // Near plane value
uniform float u_farPlane;   // Far plane value

void main() {
    // Sample depth from the stored depth map
    float depth = texture(u_depthMap, texCoords).r;
	// Remap depth to [0, 1] range based on near and far planes
    float normalizedDepth = (depth - u_nearPlane) / (75 - u_nearPlane);

    // Make sure the depth is clamped to the range [0, 1]
    normalizedDepth = 1 - clamp(normalizedDepth, 0.0, 1.0);
    // normalized CoC 0@in-focus → 1@max out-of-focus
    float coc = clamp(abs(normalizedDepth - u_focusDist) / u_focusRange, 0.0, 1.0);

    vec3 sharp   = texture(u_scene, texCoords).rgb;
    vec3 blurred = texture(u_blur,  texCoords).rgb;
    vec3 color   = mix(sharp, blurred, coc);

    fragColor = vec4(color, 1.0);
}
