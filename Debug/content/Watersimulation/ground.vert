#version 430 core

layout (location = 0) in vec4 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vTexCoord;

out vec4 fModelPosition;
out vec4 fWorldPosition;
out vec3 fNormal;
out vec2 fTexCoord;

layout(location = 0) uniform mat4 WorldMatrix;
layout(location = 1) uniform mat4 ViewMatrix;
layout(location = 2) uniform mat4 ProjectionMatrix;
layout(location = 3) uniform mat3 NormalMatrix;

void main()
{
	fModelPosition = vPosition;
	fTexCoord = vTexCoord;
	fNormal = NormalMatrix * vNormal;
	fWorldPosition = WorldMatrix * vPosition;
	gl_Position = ProjectionMatrix * ViewMatrix * fWorldPosition;
}