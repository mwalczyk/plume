#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (set = 0, binding = 0) uniform UniformBufferObject
{
	mat4 model;
	mat4 view;
	mat4 projection;
} ubo;

// Vertex shader inputs
layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec2 inTexcoord;

// Vertex shader outputs
out gl_PerVertex
{
	vec4 gl_Position;
};

layout (location = 0) out vec2 vsTexcoord;

void main()
{
	vsTexcoord = inTexcoord;

  gl_Position = vec4(inPosition, 1.0);
}
