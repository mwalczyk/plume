#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (set = 0, binding = 0) uniform UniformBufferObject
{
	mat4 model;
	mat4 view;
	mat4 projection;
} ubo;

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
	float a = constants.mouse.x;
	float b = constants.position.x;
	vec3 newColor = vsColor;
	newColor.r = a;
	oColor = vec4(newColor * sin(constants.time), 1.0);
}
