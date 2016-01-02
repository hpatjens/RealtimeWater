#version 430 core

in vec4 vPosition;
in vec4 vTexCoord;

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