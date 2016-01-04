#version 430 core

layout (location = 0) in vec4 vPosition;
layout (location = 2) in vec2 vTexCoord;

out vec2 fTexCoord;

void main() {
	fTexCoord = vTexCoord;
	gl_Position = vPosition;
}