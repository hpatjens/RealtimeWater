#version 330 core

vec3 gridPosition(int gridSideWidth, int vertexID)
{
	int perLayer = gridSideWidth * gridSideWidth;
	
	int y = vertexID / perLayer;
	int onlayer = vertexID % perLayer;
	
	int x = onlayer / gridSideWidth;
	int z = onlayer % gridSideWidth;
	
	return vec3(float(x) / (0.5 * gridSideWidth) - 1, 
			    float(y) / (0.5 * gridSideWidth) - 1, 
			    float(z) / (0.5 * gridSideWidth) - 1);
}