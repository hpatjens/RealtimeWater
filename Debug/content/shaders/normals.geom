#version 430 core

layout (triangles) in;
layout (line_strip, max_vertices = 2) out;

in vec4 gPosition[];
in vec3 gNormal[];

layout(location = 0) uniform mat4 WorldMatrix;
layout(location = 1) uniform mat4 ViewMatrix;
layout(location = 2) uniform mat4 ProjectionMatrix;
layout(location = 3) uniform mat3 NormalMatrix;
layout(location = 4) uniform float NormalLength;

void main() {
	for (int i = 0; i < gl_in.length(); ++i) {
		vec3 normal = gNormal[i] * 0.01;
		vec3 worldNormal = NormalMatrix * normal;
		
		vec4 worldPosition1 = WorldMatrix * gPosition[i];
		vec4 worldPosition2 = WorldMatrix * (gPosition[i] + vec4(worldNormal, 0));
		
		gl_Position = ProjectionMatrix * ViewMatrix * worldPosition1;
		EmitVertex();
		
		gl_Position = ProjectionMatrix * ViewMatrix * worldPosition2;
		EmitVertex();
		
		EndPrimitive();
	}	
}