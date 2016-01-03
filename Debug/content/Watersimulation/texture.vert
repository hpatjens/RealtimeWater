#version 430 core

layout (location = 0) in vec4 vPosition;
layout (location = 2) in vec2 vTexCoord;

out vec3 fNormal;
out vec2 fTexCoord;

layout(location = 0) uniform mat4 WorldMatrix;
layout(location = 1) uniform mat4 ViewMatrix;
layout(location = 2) uniform mat4 ProjectionMatrix;

void main()
{
	fTexCoord = vTexCoord;
	gl_Position = ProjectionMatrix * ViewMatrix * WorldMatrix * vPosition;
}