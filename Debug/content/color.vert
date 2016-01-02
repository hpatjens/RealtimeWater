#version 330 core

in vec4 vPosition;

void main()
{
	gl_Position = vec4(vPosition);
}