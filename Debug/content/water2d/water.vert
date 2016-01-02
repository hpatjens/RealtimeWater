#version 430 core

in vec4 vPosition;
in vec3 vNormal;
in vec2 vTexCoord;
in ivec2 vVertexPosition;

out vec4 fWorldPosition;
out vec3 fNdc;
out vec3 fNormal;
out vec2 fTexCoord;
out float fVelocity;

uniform mat4 ProjectionMatrix;
uniform mat4 ViewMatrix;
uniform mat4 WorldMatrix;
uniform mat3 NormalMatrix;

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