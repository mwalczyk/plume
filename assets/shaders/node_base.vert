#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (set = 0, binding = 0) uniform UniformBufferObject
{
	mat4 model;
	mat4 view;
	mat4 projection;
} ubo;

layout (location = 0) in vec3 inPosition;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main()
{
  gl_Position = ubo.projection * ubo.model * vec4(inPosition, 1.0);
}
