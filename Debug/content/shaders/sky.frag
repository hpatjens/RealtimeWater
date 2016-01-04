#version 430 core

in vec4 fPosition;
in vec4 fTexCoord;

out vec4 FragColor;

uniform mat4 WorldMatrix;
uniform mat4 ViewMatrix;
uniform mat4 ProjectionMatrix;
layout(location = 3) uniform mat4 InverseViewProjectionMatrix;

layout(binding = 0) uniform samplerCube SkyCubemap;

void main() {
	vec4 viewRayFront = vec4(fTexCoord.xy * 2.0 - 1.0, -1.0, 1.0);
	vec4 viewRayBack  = vec4(fTexCoord.xy * 2.0 - 1.0,  1.0, 1.0);
		
	vec4 viewRayFrontWorld = InverseViewProjectionMatrix * viewRayFront;
	vec4 viewRayBackWorld  = InverseViewProjectionMatrix * viewRayBack;
	
	vec3 viewRayFrontWorldWDiv = viewRayFrontWorld.xyz / viewRayFrontWorld.w;
	vec3 viewRayBackWorldWDiv  = viewRayBackWorld.xyz / viewRayBackWorld.w;
	
	vec3 viewRayWorld = -normalize(viewRayFrontWorldWDiv - viewRayBackWorldWDiv);
	
	FragColor = texture(SkyCubemap, viewRayWorld);
}