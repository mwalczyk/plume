#version 450
#extension GL_ARB_separate_shader_objects : enable

// Fragment shader inputs
layout(location = 0) in vec3 vsColor;

// Fragment shader outputs
layout(location = 0) out vec4 oColor;

layout(std430, push_constant) uniform PushConstants
{
	float time;
	vec2 mouse;
} constants;

void main()
{
	float t = sin(constants.time);
	float m = constants.mouse.x;
	vec3 modified = vec3(m, vsColor.gb);
	oColor = vec4(modified, 1.0);
}
