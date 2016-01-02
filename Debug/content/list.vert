#version 430 core

in float vPressure;

out float gPressure;
out int gVertexID;

void main()
{
	gPressure = vPressure;
	gVertexID = gl_VertexID;
}