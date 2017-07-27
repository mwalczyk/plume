#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (set = 0, binding = 1) uniform sampler3D sfd_map;

// Fragment shader inputs
layout(location = 0) in vec2 vs_texcoord;

// Fragment shader outputs
layout(location = 0) out vec4 o_color;

layout(std430, push_constant) uniform push_constants
{
	float time;
	vec2 mouse;
	vec3 color;
} constants;

void main()
{
	const float number_of_layers = 32.0;
	const float grid = ceil(pow(number_of_layers, 0.5));

	vec2 uv = vs_texcoord;
	uv *= grid;
	vec2 fpos = fract(uv);
	vec2 ipos = floor(uv);
	uv = fpos;

	float layer = ipos.y * grid + ipos.x;

	float t = sin(constants.time);
	float m = constants.mouse.x;
	vec3 c = constants.color;

	vec3 samp = texture(sfd_map, vec3(uv, layer)).rgb;
	//vec3(uv, layer)
	o_color = vec4(samp * (layer / number_of_layers), 1.0);
	o_color.r *= t * 0.5 + 0.5;
}
