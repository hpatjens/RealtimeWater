#version 430 core

in vec2 fTexCoord;

out vec4 FragColor;

layout(binding = 0) uniform sampler2D Texture;

void main() {	
	FragColor = texture2D(Texture, fTexCoord);
}




















