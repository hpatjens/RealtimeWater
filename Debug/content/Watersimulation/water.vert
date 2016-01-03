#version 430 core

layout(location = 0) in vec4 vPosition;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vTexCoord;
layout(location = 3) in ivec2 vVertexPosition;

out vec4 fWorldPosition;
out vec3 fNdc;
out vec3 fNormal;
out vec2 fTexCoord;
out float fVelocity;

layout(location = 0) uniform mat4 WorldMatrix;
layout(location = 1) uniform mat4 ViewMatrix;
layout(location = 2) uniform mat4 ProjectionMatrix;
layout(location = 3) uniform mat3 NormalMatrix;

void main()
{
	fTexCoord = vTexCoord;
	fNormal = NormalMatrix * vNormal;
	fWorldPosition = WorldMatrix * vPosition;
	fVelocity = vPosition.w;
	vec4 dc = ProjectionMatrix * ViewMatrix * fWorldPosition;
	fNdc = dc.xyz / dc.w;
	gl_Position = dc;
}