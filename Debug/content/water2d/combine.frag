#version 430 core

in vec2 fTexCoord;

out vec4 FragColor;

uniform sampler2D BackgroundTexture;
uniform sampler2D BackgroundDepthTexture;
uniform sampler2D WaterTexture;
uniform sampler2D WaterDepthTexture;

void main()
{	
	if (texture2D(WaterDepthTexture, fTexCoord).r < texture2D(BackgroundDepthTexture, fTexCoord).r)
	{
		FragColor = texture2D(WaterTexture, fTexCoord);
	}
	else
	{
		FragColor = texture2D(BackgroundTexture, fTexCoord);		
	}
}




















