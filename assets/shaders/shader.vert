#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (set = 0, binding = 0) uniform UniformBufferObject
{
	mat4 model;
} ubo;

// Vertex shader inputs
layout (location = 0) in vec2 inPosition;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inTexcoord;

// Vertex shader outputs
out gl_PerVertex
{
	vec4 gl_Position;
};

layout (location = 0) out vec3 vsColor;
layout (location = 1) out vec2 vsTexcoord;

void main()
{
	vsColor = inColor;
	vsTexcoord = inTexcoord;
  gl_Position = ubo.model * vec4(inPosition, 0.0, 1.0);
}
