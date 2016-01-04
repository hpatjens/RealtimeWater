#version 430 core

in vec3 fNormal;
in vec2 fTexCoord;
in vec4 fWorldPosition;
in vec3 fNdc;
in float fVelocity;

out vec4 FragColor;

uniform mat4 ViewMatrix;
uniform mat4 ProjectionMatrix;

layout(location = 4) uniform vec2 FramebufferSize;
layout(location = 5) uniform mat4 TopViewMatrix;
layout(location = 6) uniform mat4 TopProjectionMatrix;
layout(location = 7) uniform float DeltaTime;
layout(location = 8) uniform float Time;

layout(binding = 0) uniform sampler2D BackgroundColorTexture;
layout(binding = 1) uniform sampler2D BackgroundDepthTexture;
layout(binding = 2) uniform sampler2D TopViewDepthTexture;
layout(binding = 3) uniform sampler2D NoiseTexture;
layout(binding = 4) uniform sampler2D NoiseNormalTexture;
layout(binding = 5) uniform sampler1D SubSurfaceScatteringTexture;
layout(binding = 6) uniform samplerCube SkyCubemap;

float inverseDepthRangeTransformation(float depth) {
    return (2.0 * depth - gl_DepthRange.near - gl_DepthRange.far) /
            (gl_DepthRange.far - gl_DepthRange.near);
}

vec3 positionFromDepth(vec2 texCoord, sampler2D depthTexture, mat4 inverseViewProjection) {
    vec4 n = vec4(texCoord * 2.0 - 1.0, 0.0, 0.0);
    float depth = texture2D(depthTexture, texCoord).r;
    n.z = inverseDepthRangeTransformation(depth);
    n.w = 1.0;
    vec4 worldPosition = inverseViewProjection * n;

    return worldPosition.xyz / worldPosition.w;
}

vec4 getBackgroundColor(vec2 coordinate) {
	return texture2D(BackgroundColorTexture, coordinate);
}

float getBackgroundDepth(vec2 coordinate) {
	return texture2D(BackgroundDepthTexture, coordinate).r;
}

vec3 getTopWorldPosition(vec4 worldPosition) {
	vec4 dc = TopProjectionMatrix * TopViewMatrix * worldPosition;
	vec3 ndc = dc.xyz / dc.w;
	vec2 coordinate = (ndc.xy + 1.0) / 2.0;
	mat4 inverseViewProjection = inverse(TopProjectionMatrix * TopViewMatrix);
	return positionFromDepth(coordinate, TopViewDepthTexture, inverseViewProjection);
}

vec3 getBackgroundWorldPosition(vec2 texCoord) {
	mat4 inverseViewProjection = inverse(ProjectionMatrix * ViewMatrix);
	return positionFromDepth(texCoord, BackgroundDepthTexture, inverseViewProjection);
}

void main() {
	vec2 normalizedFragCoord = gl_FragCoord.xy / FramebufferSize;

	if (gl_FragCoord.z >= getBackgroundDepth(normalizedFragCoord)) {
		discard;
	}
	
	// Noising the normal to create small ripples.
	const float NORMAL_NOISE_TEXTURE_STRETCH = 2;
	const float NORMAL_NOISE_STRENGTH = 0.5;		
	const float NOISE_MOVEMENT_SPEED = 0.01;
	const vec2 NOISE_MOVEMENT_DIRECTION = vec2(0.13, 0.57);	
	vec2 noiseTexCoord = fTexCoord + Time * NOISE_MOVEMENT_SPEED * NOISE_MOVEMENT_DIRECTION;
	vec3 noiseNormal = texture2D(NoiseNormalTexture, noiseTexCoord * NORMAL_NOISE_TEXTURE_STRETCH).xyz;
	vec3 normal = normalize(fNormal + NORMAL_NOISE_STRENGTH * noiseNormal);
	
	// Depth from viewer
	vec3 backgroundWorldPosition = getBackgroundWorldPosition(normalizedFragCoord);
	float depthWorld = length(backgroundWorldPosition - fWorldPosition.xyz / fWorldPosition.w);
		
	// Lookup of light color based on depth
	const float LIGHT_SCATTERING_DEPTH_FACTOR = 2;
	vec4 lightScattering = texture(SubSurfaceScatteringTexture, depthWorld * LIGHT_SCATTERING_DEPTH_FACTOR);
		
	// Scattering light
	const float SCATTERING_INTENSITY = 0.5;
	vec4 subsurfaceLight = SCATTERING_INTENSITY * lightScattering;
	
	// Eye vector
	mat4 inverseViewProjection = inverse(ProjectionMatrix * ViewMatrix);
	vec4 eyeVectorFront = inverseViewProjection * vec4(fNdc.x, fNdc.y, -1, 1);
	vec4 eyeVectorBack  = inverseViewProjection * vec4(fNdc.x, fNdc.y,  1, 1);	
	vec3 eyeVectorFrontWDiv = eyeVectorFront.xyz / eyeVectorFront.w;
	vec3 eyeVectorBackWDiv  = eyeVectorBack.xyz / eyeVectorBack.w;	
	vec3 eyeVector = normalize(eyeVectorFrontWDiv - eyeVectorBackWDiv);	
	
	// Refraction
	const float REFRACTION_STRENGTH = 0.5;
	const float REFRACTION_MAX_DEPTH = 0.5;
	float refractionOffset = min(REFRACTION_MAX_DEPTH, depthWorld);
	vec4 background = getBackgroundColor(normalizedFragCoord + normal.xz * REFRACTION_STRENGTH * refractionOffset);
	vec4 refraction = mix(background, subsurfaceLight, SCATTERING_INTENSITY);
	
	// Reflection
	vec3 reflectionVector = reflect(-eyeVector, normal);	
	vec4 reflection = 0.8 * texture(SkyCubemap, reflectionVector);
	
	// Fresnel-Term
	float fresnel = dot(eyeVector, normal);

	vec4 waterColor = mix(reflection, refraction, fresnel);
	
	FragColor = mix(background, waterColor, min(1, 100 * depthWorld));
}




















