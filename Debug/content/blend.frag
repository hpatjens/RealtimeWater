#version 430 core

uniform sampler2D head_pointer_image;

//uniform samplerBuffer list_buffer;

#define MAX_FRAGMENTS 10

//uvec4 fragments[MAX_FRAGMENTS];

out vec4 FragColor;

in float fPressure;
in vec3 fNormal;

//int build_local_fragment_list()
//{
//	int current;
//	int frag_count = 0;
//	
//	current = int(texelFetch(head_pointer_image, ivec2(gl_FragCoord.xy), 0).x);
//	
//	while (current != 0xFFFFFFFF && frag_count < MAX_FRAGMENTS)
//	{
//		uvec4 item = uvec4(texelFetch(list_buffer, current));
//		
//		current = int(item.x);
//		
//		fragments[frag_count] = item;			
//	
//		frag_count++;
//	}
//	
//	return 1; // TODO
//}
//
//vec4 calculate_final_color(int frag_count)
//{
//	vec4 final_color = vec4(0.0);
//	
//	for (int i = 0; i < frag_count; ++i)
//	{	
//		vec4 frag_color = unpackUnorm4x8(fragments[i].y);
//		
//		final_color = mix(final_color, frag_color, 0.5);
//	}
//	
//	return final_color;
//}

void main()
{
	//int frag_count = build_local_fragment_list();
	
	//FragColor = calculate_final_color(frag_count);
	
	//FragColor = texelFetch(head_pointer_image, ivec2(gl_FragCoord.xy), 0);
	//FragColor = unpackUnorm4x8(fragments[0].y);
	FragColor = texture2D(head_pointer_image, vec2(gl_FragCoord.x / 1200.0, gl_FragCoord.y / 800.0));
	//FragColor = vec4(gl_FragCoord.x / 1200.0, gl_FragCoord.y / 800.0, 0, 1);
	//FragColor = vec4(1, 0, 1, 1) * min(dot(normalize(vec3(1, 1.5, 1.7)), fNormal) + vec4(0.2), 1.0);	
}