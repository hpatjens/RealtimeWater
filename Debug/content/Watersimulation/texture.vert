#version 430 core

layout (location = 0) in vec4 vPosition;
layout (location = 2) in vec2 vTexCoord;

out vec3 fNormal;
out vec2 fTexCoord;

uniform mat4 ProjectionMatrix;
uniform mat4 ViewMatrix;
uniform mat4 WorldMatrix;

void main()
{
	fTexCoord = vTexCoord;
	gl_Position = ProjectionMatrix * ViewMatrix * WorldMatrix * vPosition;
}