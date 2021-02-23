#version 330 core
layout(location = 0) in vec3 coord3d;

uniform mat4 uniform_projection_matrix;

void main(void) 
{
	glm::mat4 biasMatrix(
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 0.5, 0.0,
	0.5, 0.5, 0.5, 1.0
	);

	glm::mat4 biasMVP = biasMatrix * uniform_projection_matrix;

	gl_Position = uniform_projection_matrix * vec4(coord3d, 1.0);

	ShadowCoord = biasMVP * vec4(coord3d, 1.0);
}
