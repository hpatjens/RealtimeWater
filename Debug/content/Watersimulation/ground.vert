#version 430 core

in vec4 vPosition;
in vec3 vNormal;
in vec2 vTexCoord;

out vec4 fModelPosition;
out vec4 fWorldPosition;
out vec3 fNormal;
out vec2 fTexCoord;

uniform mat4 ProjectionMatrix;
uniform mat4 ViewMatrix;
uniform mat4 WorldMatrix;
uniform mat3 NormalMatrix;

void main()
{
	fModelPosition = vPosition;
	fTexCoord = vTexCoord;
	fNormal = NormalMatrix * vNormal;
	fWorldPosition = WorldMatrix * vPosition;
	gl_Position = ProjectionMatrix * ViewMatrix * fWorldPosition;
}