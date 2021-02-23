#version 330 core

layout(location = 0) out float fragDepth;

void main(void)
{
	fragDepth = gl_FragCoord.z;
}