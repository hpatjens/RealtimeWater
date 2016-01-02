#version 430 core

layout (location = 0) in vec4 vPosition;
layout (location = 2) in vec4 vTexCoord;

out vec4 fPosition;
out vec4 fTexCoord;

uniform mat4 ProjectionMatrix;
uniform mat4 ViewMatrix;
uniform mat4 WorldMatrix;

void main()
{
	fTexCoord = vTexCoord;
	fPosition = vPosition;
	gl_Position = fPosition;
}