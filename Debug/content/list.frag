#version 430 core

layout (early_fragment_tests) in;

layout (binding = 0, offset = 0) uniform atomic_uint index_counter;

layout (binding = 0, rgba32ui) uniform uimageBuffer list_buffer; // u added

layout (binding = 1, r32ui) uniform uimage2DRect head_pointer_image; // was type imageRect

in float fPressure;
in vec3 fNormal;

out vec4 FragColor;

void main()
{
	if (fPressure > 0.5)
	{
		vec4 frag_color = vec4(0, 1, 0, 1);
		
		uint new_head = atomicCounterIncrement(index_counter);
		
		uint old_head = imageAtomicExchange(head_pointer_image, ivec2(gl_FragCoord.xy), new_head);
		
		uvec4 item;
		item.x = old_head;
		item.y = packUnorm4x8(frag_color);
		item.z = floatBitsToUint(gl_FragCoord.z);
		item.w = 0;
		
		imageStore(list_buffer, int(new_head), item);
		//imageStore(list_buffer, int(new_head), uvec4(0, 1, 0, 1));
		
		//FragColor = vec4(1, 1, 0, 1) * min(dot(normalize(vec3(1, 1.5, 1.7)), fNormal) + vec4(0.2), 1.0);
	}
	else
	{
		discard;
	}
}