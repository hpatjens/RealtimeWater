#version 430 core

in vec4 fModelPosition;
in vec4 fWorldPosition;
in vec3 fNormal;
in vec2 fTexCoord;

out vec4 FragColor;

layout(location = 4) uniform mat4 WaterMapProjectionMatrix;
layout(location = 5) uniform mat4 WaterMapViewMatrix;
layout(location = 6) uniform vec3 LightPosition;
layout(location = 7) uniform float TextureScale;
layout(location = 8) uniform float Time;

layout(binding = 0) uniform sampler2D WaterMapDepth;
layout(binding = 1) uniform sampler2D WaterMapNormals;
layout(binding = 2) uniform sampler2D Texture;
layout(binding = 3) uniform sampler2D NoiseNormalTexture;
layout(binding = 4) uniform sampler2D CausticTexture;
layout(binding = 5) uniform sampler1D SubSurfaceScatteringTexture;

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

vec3 getWaterWorldPosition(vec2 texCoord) {
	mat4 inverseViewProjection = inverse(WaterMapProjectionMatrix * WaterMapViewMatrix);
	return positionFromDepth(texCoord, WaterMapDepth, inverseViewProjection);
}

vec3 decodeNormal(vec4 normal) {
    return vec3(normal.xyz * 2.0 - 1.0);
}

void main() {
	// Projection the world position onto the watermap
	vec4 waterMapCoordinate = WaterMapProjectionMatrix * WaterMapViewMatrix * fWorldPosition;
	vec2 waterMapCoordinateWDiv = waterMapCoordinate.xy / waterMapCoordinate.w;
	vec2 normalizedWaterMapCoordinate = (waterMapCoordinateWDiv + vec2(1.0)) * 0.5;	
	vec3 waterWorldPosition = getWaterWorldPosition(normalizedWaterMapCoordinate);
	float waterZDepth = inverseDepthRangeTransformation(texture2D(WaterMapDepth, normalizedWaterMapCoordinate).r);
	float waterDepth = length(waterWorldPosition - fWorldPosition.xyz / fWorldPosition.w);	
		
	// Light on ground, which is assumed to be approximately the light on the water surface
	const vec3 SUN_LIGHT_VECTOR = normalize(vec3(1, 0.5, 0.7));
	const vec4 SUN_LIGHT_COLOR = vec4(1);
	const vec4 AMBIENT_LIGHT_INTENSITY = vec4(vec3(0.2), 1.0);	
	float diffuseIntensity = max(0, dot(fNormal, SUN_LIGHT_VECTOR));
	vec4 diffuseLight = SUN_LIGHT_COLOR * diffuseIntensity;
	vec4 surfaceLight = diffuseLight + AMBIENT_LIGHT_INTENSITY;
	
	// Ground texture
	vec4 textureColor = texture2D(Texture, fTexCoord * TextureScale);

	// Above or below water surface
	if (waterMapCoordinate.z > waterZDepth) { // Under water 
		// Attenuation due to participating media
		const float DENSITY = 8;
		float attenuation = exp(-DENSITY * waterDepth); // 1 / attenuation!

		// Lookup of light color based on depth
		const float LIGHT_SCATTERING_DEPTH_FACTOR = 2;
		vec4 lightScattering = texture(SubSurfaceScatteringTexture, waterDepth * LIGHT_SCATTERING_DEPTH_FACTOR);
		
		// Scattering light
		const float SCATTERING_INTENSITY = 0.5;
		vec4 subsurfaceLight = SCATTERING_INTENSITY * lightScattering * waterDepth;
		
		// Attenuation of the light on the surface due to participating media
		vec4 groundLight = attenuation * surfaceLight * mix(vec4(1), lightScattering, waterDepth);
		
		// Calculate caustic light
		const float CAUSTIC_STRENGTH = 5;
		const float CAUSTIC_DISTORTION = 0.5;
		vec3 waterNormal = decodeNormal(texture2D(WaterMapNormals, normalizedWaterMapCoordinate));
		vec2 noiseNoisingDirection = texture2D(NoiseNormalTexture, fTexCoord + vec2(sin(Time * 0.005), sin(Time * 0.01))).xz;
		vec4 causticLight = CAUSTIC_STRENGTH * waterDepth * attenuation * texture2D(CausticTexture, 20 * normalizedWaterMapCoordinate + CAUSTIC_DISTORTION * waterNormal.xz + CAUSTIC_DISTORTION * noiseNoisingDirection);
	
		FragColor = textureColor * (groundLight + subsurfaceLight + causticLight);
	} else { // Above water
		FragColor = textureColor * surfaceLight;
	}
}




















