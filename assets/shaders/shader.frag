#version 450
#extension GL_ARB_separate_shader_objects : enable

// Fragment shader inputs
layout(location = 0) in vec3 vsColor;

// Fragment shader outputs
layout(location = 0) out vec4 oColor;


layout (set = 0, binding = 1) uniform sampler2D CombinedImageSampler;

layout(std430, push_constant) uniform PushConstants
{
	float time;
	vec2 mouse;
	vec3 color;
} constants;

void main()
{
	float t = sin(constants.time);
	float m = constants.mouse.x;
	vec4 sampledColor = texture(CombinedImageSampler, vec2(0.0, 0.0));
	vec3 c = constants.color;

	oColor = vec4(vsColor, 1.0);
}
