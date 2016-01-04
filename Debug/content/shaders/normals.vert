#version 430 core

layout (location = 0) in vec4 vPosition;
layout (location = 1) in vec3 vNormal;

out vec4 gPosition;
out vec3 gNormal;

void main() {	
	gNormal = vNormal;
	gPosition = vPosition;	
	gl_Position = vPosition;
}