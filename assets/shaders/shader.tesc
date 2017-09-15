#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (vertices = 3) out;

layout (location = 0) in vec2 vs_texcoord[];

layout (location = 0) out vec2 tc_texcoord[3];

void main()
{
    if (gl_InvocationID == 0)
    {
        gl_TessLevelInner[0] = 1.0;
        gl_TessLevelOuter[0] = 1.0;
        gl_TessLevelOuter[1] = 1.0;
        gl_TessLevelOuter[2] = 1.0;
    }

    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;

    tc_texcoord[gl_InvocationID] = vs_texcoord[gl_InvocationID];
}
