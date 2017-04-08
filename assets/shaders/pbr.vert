#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (set = 0, binding = 0) uniform UniformBufferObject
{
	mat4 model;
	mat4 view;
	mat4 projection;
} ubo;

// Vertex shader inputs
layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_color;
layout (location = 2) in vec3 in_normal;
layout (location = 3) in vec3 in_instance_position;

// Vertex shader outputs
out gl_PerVertex
{
	vec4 gl_Position;
};

layout (location = 0) out vec3 vs_world_position;
layout (location = 1) out vec3 vs_color;
layout (location = 2) out vec3 vs_normal;
layout (location = 3) flat out uint vs_instance_id;

void main()
{
	mat4 normal_matrix = transpose(inverse(ubo.view * ubo.model));

	vec3 offset_position = in_position + in_instance_position;
	vs_world_position = (ubo.model * vec4(offset_position, 1.0)).xyz;
	vs_color = in_color;
	vs_normal = mat3(normal_matrix) * in_normal;
	vs_instance_id = gl_InstanceIndex;

  gl_Position = ubo.projection * ubo.view * ubo.model * vec4(offset_position, 1.0);
}
