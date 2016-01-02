#version 330 core

in float fPressure;
in vec3 fNormal;
in vec4 fCellMiddle;

out vec4 FragColor;

void main()
{
	FragColor = vec4(1, 1, 0, 1) * min(dot(normalize(vec3(1, 1.5, 1.7)), fNormal) + vec4(0.2), 1.0);	
}