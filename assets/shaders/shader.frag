#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 vsColor;

layout(location = 0) out vec4 oColor;

layout(std430, push_constant) uniform PushConstants
{
	float time;
} constants;

void main()
{
	oColor = vec4(vsColor * sin(constants.time), 1.0);
}
