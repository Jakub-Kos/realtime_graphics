#version 430 core
layout(local_size_x = 16, local_size_y = 16) in;


layout(rgba32f, binding = 0) uniform readonly image2D inputImage;
layout(rgba32f, binding = 1) uniform writeonly image2D outputImage;


const float kernel[25] = float[](
	1.0/273,  4.0/273,  7.0/273,  4.0/273, 1.0/273,
	4.0/273, 16.0/273, 26.0/273, 16.0/273, 4.0/273,
	7.0/273, 26.0/273, 41.0/273, 26.0/273, 7.0/273,
	4.0/273, 16.0/273, 26.0/273, 16.0/273, 4.0/273,
	1.0/273,  4.0/273,  7.0/273,  4.0/273, 1.0/273
);


void main() {
	ivec2 texSize = imageSize(inputImage);
	ivec2 gid = ivec2(gl_GlobalInvocationID.xy);

	if (gid.x >= texSize.x || gid.y >= texSize.y) {
		return; // Skip out-of-bounds work items
	}

	// accumulate weighted sum
    vec4 sum = vec4(0.0);
    for (int dy = -2; dy <= 2; ++dy) {
        for (int dx = -2; dx <= 2; ++dx) {
            // compute flat index into 5×5 kernel
            int kIdx = (dy + 2) * 5 + (dx + 2);
            // clamp to edge
            ivec2 coord = clamp(gid + ivec2(dx, dy),
                                ivec2(0, 0),
                                texSize - ivec2(1, 1));
            vec4 sample = imageLoad(inputImage, coord);
            sum += sample * kernel[kIdx];
        }
    }

    // write blurred result
    imageStore(outputImage, gid, sum, 1.0);
}
