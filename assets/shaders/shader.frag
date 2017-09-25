#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (set = 0, binding = 1) uniform sampler3D sfd_map;

// Inputs
layout (location = 0) in vec3 te_normal;
layout (location = 1) in vec2 te_texcoord;
layout (location = 2) in float te_noise;

// Outputs
layout (location = 0) out vec4 o_color;

layout(std430, push_constant) uniform push_constants
{
	float time;
	vec2 mouse;
} constants;

void main()
{
    float p = pow(te_noise, 0.5);
	o_color = vec4(vec3(p), 1.0);
}
