#version 430 core

in vec4 vPosition;
in vec2 vTexCoord;

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