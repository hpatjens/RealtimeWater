#version 430 core

in vec2 fTexCoord;

out vec4 FragColor;

layout(binding = 0) uniform sampler2D BackgroundColorTexture;
layout(binding = 1) uniform sampler2D BackgroundDepthTexture;
layout(binding = 2) uniform sampler2D WaterColorTexture;
layout(binding = 3) uniform sampler2D WaterDepthTexture;

void main() {
	float waterDepth = texture2D(WaterDepthTexture, fTexCoord).r;
	float backgroundDepth = texture2D(BackgroundDepthTexture, fTexCoord).r;
	
	if (waterDepth < backgroundDepth) {
		FragColor = texture2D(WaterColorTexture, fTexCoord);
	} else {
		FragColor = texture2D(BackgroundColorTexture, fTexCoord);		
	}
}




















