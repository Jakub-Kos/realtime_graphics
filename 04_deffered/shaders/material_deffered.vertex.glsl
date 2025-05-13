#version 430 core


uniform mat4 u_modelMat;
uniform mat4 u_viewMat;
uniform mat4 u_projMat;
uniform mat3 u_normalMat;

// uniform mat4 u_lightMat;

in vec3 in_vert;
in vec3 in_normal;
in vec2 in_texCoords;

out vec4 position;
out vec2 texCoords;
out vec3 normal;

out vec4 viewPosition;  // This will store the position in camera space (view space)
out vec4 shadowCoords;



void main(void)
{
	position = u_modelMat * vec4(in_vert, 1);
	
	// Transform the position from world space to view space
    viewPosition = u_viewMat * position; // Now in view space
	
	normal = normalize(u_normalMat * in_normal);
	texCoords = in_texCoords;


	gl_Position = u_projMat * u_viewMat * position;

	// shadowCoords = u_lightMat * position;
}

