#version 430 core

layout (triangles) in;
layout (line_strip, max_vertices = 2) out;

in vec4 gPosition[];
in vec3 gNormal[];

uniform mat4 ProjectionMatrix;
uniform mat4 ViewMatrix;
uniform mat4 WorldMatrix;
uniform mat3 NormalMatrix;

uniform float NormalLength;

void main()
{	
	for (int i = 0; i < gl_in.length(); ++i)
	{
		vec3 normal = gNormal[i] * 0.01;
		vec3 worldNormal = NormalMatrix * normal;
		
		//vec3 position = gPosition[i].xyz / gPosition[i].w;
		
		vec4 worldPosition1 = WorldMatrix * gPosition[i];
		vec4 worldPosition2 = WorldMatrix * (gPosition[i] + vec4(worldNormal, 0));
		
		gl_Position = ProjectionMatrix * ViewMatrix * worldPosition1;
		EmitVertex();
		
		gl_Position = ProjectionMatrix * ViewMatrix * worldPosition2;
		EmitVertex();
		
		EndPrimitive();
	}
	
}