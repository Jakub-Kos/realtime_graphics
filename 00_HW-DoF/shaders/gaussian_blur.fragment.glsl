#version 430 core
layout(binding = 0) uniform sampler2D u_1image;
layout(binding = 1) uniform sampler2D u_2image;
layout(location = 0) uniform bool u_horizontal;

in vec2 texCoords;
layout(location = 0) out vec4 outBlur1;
layout(location = 1) out vec4 outBlur2;

const float weights[5] = float[](0.22703, 0.19459, 0.12162, 0.05405, 0.01622);

vec3 gaussianBlur(sampler2D img, vec2 baseTexel, bool horiz) {
    vec3 sum = texture(img, texCoords).rgb * weights[0];
    for (int i = 1; i < 5; ++i) {
        vec2 off = horiz
            ? vec2(baseTexel.x * i, 0.0)
            : vec2(0.0, baseTexel.y * i);
        sum += texture(img, texCoords + off).rgb * weights[i];
        sum += texture(img, texCoords - off).rgb * weights[i];
    }
    return sum;
}

// simple Gaussian function
float gaussian(float x, float sigma) {
    return exp(-0.5 * (x*x) / (sigma*sigma)) / (sigma * sqrt(6.28318530718));
}

// generic separable blur for any radius
vec3 separableBlur(sampler2D img, vec2 texelSize, bool horiz, int radius) {
    vec3  sum       = vec3(0.0);
    float weightSum = 0.0;
    float sigma     = float(radius) * 0.5;  // can be tweaked

    for (int i = -radius; i <= radius; ++i) {
        float w    = gaussian(float(i), sigma);
        vec2  off  = horiz
            ? vec2(texelSize.x * float(i), 0.0)
            : vec2(0.0, texelSize.y * float(i));
        sum       += texture(img, texCoords + off).rgb * w;
        weightSum += w;
    }
    return sum / weightSum;
}

void main() {
    // 1) normal blur of u_1image
    vec2 texel1 = 1.0 / vec2(textureSize(u_1image, 0));
    vec3 blur1 = gaussianBlur(u_1image, texel1, u_horizontal);
    outBlur1 = vec4(blur1, 1.0);
    
    // 2) “strong” blur of u_2image
    // radius = 50 → 101×101 kernel (±50)
    vec2 ts2 = 1.0 / vec2(textureSize(u_2image, 0));
    outBlur2 = vec4(separableBlur(u_2image, ts2, u_horizontal, 50), 1.0);
}