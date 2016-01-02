#version 330 core

in float vPressure;

out float fPressure;

uniform mat4 ProjectionMatrix;
uniform mat4 ViewMatrix;
uniform mat4 WorldMatrix;
uniform int GridSideWidth;
uniform float Threshold;

vec3 gridPosition(int gridSideWidth, int vertexID);

void main()
{
	fPressure = vPressure;
	
	if (vPressure > Threshold)
	{
		vec4 positionInGrid = vec4(gridPosition(GridSideWidth, gl_VertexID), 1.0);
		gl_Position = ProjectionMatrix * ViewMatrix * WorldMatrix * positionInGrid;
	}
}