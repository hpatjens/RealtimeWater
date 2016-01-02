#version 430 core

in vec4 vPosition;
in vec3 vNormal;

out vec4 gPosition;
out vec3 gNormal;

void main()
{	
	gNormal = vNormal;
	gPosition = vPosition;	
	gl_Position = vPosition;
}