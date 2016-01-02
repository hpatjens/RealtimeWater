#version 430 core

layout (points) in;
layout (triangle_strip, max_vertices = 24) out;

in float gPressure[];
in int gVertexID[];

out float fPressure;
out vec3 fNormal;

uniform mat4 ProjectionMatrix;
uniform mat4 ViewMatrix;
uniform mat4 WorldMatrix;
uniform int GridSideWidth;
uniform float Threshold;

vec3 gridPosition(int gridSideWidth, int vertexID);

void main()
{
	fPressure = gPressure[0];
	
	if (gPressure[0] > Threshold)
	{
		mat4 MVP = ProjectionMatrix * ViewMatrix * WorldMatrix;
	
		vec4 positionInGrid = vec4(gridPosition(GridSideWidth, gVertexID[0]), 1.0);
		gl_Position = ProjectionMatrix * ViewMatrix * WorldMatrix * positionInGrid;
		
		float offset = 1.0 / GridSideWidth; // half cell
		
		// positive-y plane
		fNormal = vec3(0, 1, 0);
		gl_Position = MVP * (positionInGrid + vec4(-offset, +offset, +offset, 0));		
		EmitVertex();		
		gl_Position = MVP * (positionInGrid + vec4(-offset, +offset, -offset, 0));
		EmitVertex();		
		gl_Position = MVP * (positionInGrid + vec4(+offset, +offset, +offset, 0));
		EmitVertex();		
		gl_Position = MVP * (positionInGrid + vec4(+offset, +offset, -offset, 0));
		EmitVertex();
		
		EndPrimitive();
		
		// negative-y plane
		fNormal = vec3(0, -1, 0);
		gl_Position = MVP * (positionInGrid + vec4(-offset, -offset, +offset, 0));		
		EmitVertex();		
		gl_Position = MVP * (positionInGrid + vec4(-offset, -offset, -offset, 0));
		EmitVertex();		
		gl_Position = MVP * (positionInGrid + vec4(+offset, -offset, +offset, 0));
		EmitVertex();		
		gl_Position = MVP * (positionInGrid + vec4(+offset, -offset, -offset, 0));
		EmitVertex();
		
		EndPrimitive();
		
		// positive-x plane
		fNormal = vec3(1, 0, 0);
		gl_Position = MVP * (positionInGrid + vec4(+offset, +offset, +offset, 0));		
		EmitVertex();		
		gl_Position = MVP * (positionInGrid + vec4(+offset, -offset, +offset, 0));
		EmitVertex();		
		gl_Position = MVP * (positionInGrid + vec4(+offset, +offset, -offset, 0));
		EmitVertex();		
		gl_Position = MVP * (positionInGrid + vec4(+offset, -offset, -offset, 0));
		EmitVertex();
	
		EndPrimitive();
		
		// negative-x plane
		fNormal = vec3(-1, 0, 0);
		gl_Position = MVP * (positionInGrid + vec4(-offset, +offset, +offset, 0));		
		EmitVertex();		
		gl_Position = MVP * (positionInGrid + vec4(-offset, -offset, +offset, 0));
		EmitVertex();		
		gl_Position = MVP * (positionInGrid + vec4(-offset, +offset, -offset, 0));
		EmitVertex();		
		gl_Position = MVP * (positionInGrid + vec4(-offset, -offset, -offset, 0));
		EmitVertex();
	
		EndPrimitive();
		
		// positive-z plane
		fNormal = vec3(0, 0, 1);
		gl_Position = MVP * (positionInGrid + vec4(-offset, +offset, +offset, 0));		
		EmitVertex();		
		gl_Position = MVP * (positionInGrid + vec4(-offset, -offset, +offset, 0));
		EmitVertex();		
		gl_Position = MVP * (positionInGrid + vec4(+offset, +offset, +offset, 0));
		EmitVertex();		
		gl_Position = MVP * (positionInGrid + vec4(+offset, -offset, +offset, 0));
		EmitVertex();
	
		EndPrimitive();
		
		// negative-z plane
		fNormal = vec3(0, 0, -1);
		gl_Position = MVP * (positionInGrid + vec4(-offset, +offset, -offset, 0));		
		EmitVertex();		
		gl_Position = MVP * (positionInGrid + vec4(-offset, -offset, -offset, 0));
		EmitVertex();		
		gl_Position = MVP * (positionInGrid + vec4(+offset, +offset, -offset, 0));
		EmitVertex();		
		gl_Position = MVP * (positionInGrid + vec4(+offset, -offset, -offset, 0));
		EmitVertex();
	
		EndPrimitive();
	}
}