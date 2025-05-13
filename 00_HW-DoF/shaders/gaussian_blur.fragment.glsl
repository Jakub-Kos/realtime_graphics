#version 430 core
layout(binding = 0) uniform sampler2D u_image;
layout(location = 0) uniform bool u_horizontal;

in vec2 texCoords;
out vec4 fragColor;

const float weights[5] = float[](0.22703, 0.19459, 0.12162, 0.05405, 0.01622);

void main() {
    vec2 texelSize = 1.0 / vec2(textureSize(u_image, 0));
    vec3 result = texture(u_image, texCoords).rgb * weights[0];
    for(int i = 1; i < 5; ++i) {
        vec2 off = u_horizontal
            ? vec2(texelSize.x * i, 0.0)
            : vec2(0.0, texelSize.y * i);
        result += texture(u_image, texCoords + off).rgb * weights[i];
        result += texture(u_image, texCoords - off).rgb * weights[i];
    }
    fragColor = vec4(result, 1.0);
}
