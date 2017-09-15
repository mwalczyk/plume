#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (triangles) in;

layout (set = 0, binding = 0) uniform uniform_buffer_object
{
	mat4 model;
	mat4 view;
	mat4 projection;
} ubo;

layout (location = 0) in vec2 tc_texcoord[];

layout (location = 0) out vec2 te_texcoord;

void main()
{
  gl_Position = (gl_TessCoord.x * gl_in[0].gl_Position) +
                (gl_TessCoord.y * gl_in[1].gl_Position) +
                (gl_TessCoord.z * gl_in[2].gl_Position);

	//gl_Position = ubo.projection * ubo.model * gl_Position;

	te_texcoord = gl_TessCoord.x * tc_texcoord[0] + gl_TessCoord.y * tc_texcoord[1] + gl_TessCoord.z * tc_texcoord[2];
}
