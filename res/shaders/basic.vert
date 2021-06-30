#version 450

layout(location = 0) in vec4 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec3 surfaceNormal;

layout(binding = 0) uniform MVPUBO {
	mat4 mvpMatrix;
} u_MVP;

layout(location = 0) out vec4 fragColor;

void main()
{
	gl_Position = u_MVP.mvpMatrix * position;
	fragColor = color;
}

