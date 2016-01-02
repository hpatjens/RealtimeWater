#version 430 core

layout (location = 0) in vec4 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vTexCoord;

out vec3 fNormal;

uniform mat4 ProjectionMatrix;
uniform mat4 ViewMatrix;
uniform mat4 WorldMatrix;
uniform mat3 NormalMatrix;

void main()
{
	fNormal = NormalMatrix * vNormal;
	gl_Position = ProjectionMatrix * ViewMatrix * WorldMatrix * vPosition;
}