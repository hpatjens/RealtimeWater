#version 330 core

in float fPressure;

out vec4 FragColor;

void main()
{
	if (fPressure > 0.5)
		FragColor = vec4(1, 0, 0, 1);
	else
		discard;
}