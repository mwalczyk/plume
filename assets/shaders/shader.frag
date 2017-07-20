#version 450
#extension GL_ARB_separate_shader_objects : enable

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
	float t = sin(constants.time);
	float m = constants.mouse.x;
	vec3 c = constants.color;

	o_color = vec4(vs_texcoord, 1.0, 1.0);
}
