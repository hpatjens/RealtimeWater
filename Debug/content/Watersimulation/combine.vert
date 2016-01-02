#version 430 core

in vec4 vPosition;
in vec2 vTexCoord;

out vec2 fTexCoord;

void main()
{
	fTexCoord = vTexCoord;
	gl_Position = vPosition;
}