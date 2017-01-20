#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 vsColor;

layout(location = 0) out vec4 oColor;

layout(std430, push_constant) uniform PushConstants
{
	float time;
	vec2 mouse;
	vec3 position;
} constants;

void main()
{
	float t = sin(constants.time);
	float a = constants.mouse.x;
	float b = constants.position.x;
	vec3 newColor = vsColor;
	newColor.r = a;
	oColor = vec4(newColor, 1.0);
}
