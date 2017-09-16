#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (vertices = 3) out;

layout(std430, push_constant) uniform push_constants
{
	float time;
	vec2 mouse;
} constants;

// Inputs
layout (location = 0) in vec3 vs_normal[];
layout (location = 1) in vec2 vs_texcoord[];

// Outputs
layout (location = 0) out vec3 tc_normal[3];
layout (location = 1) out vec2 tc_texcoord[3];
layout (location = 2) out float tc_tess_inner[3];
layout (location = 3) out float tc_tess_outer[3];

vec2 hash2(in vec2 p)
{
    p = vec2(dot(p, vec2(12.9898, 78.233)),
             dot(p, vec2(139.234, 98.187)));

    return fract(sin(p) * 43758.5453123) * 2.0 - 1.0;
}

vec2 quintic_hermine(in vec2 x)
{
    return x * x * x * (x * (x * 6.0 - 15.0) + 10.0);
}

float noise(in vec2 p)
{
    vec2 i = floor(p);
    vec2 f = fract(p);

    // four corners
    vec2 a = hash2(i + vec2(0.0, 0.0));
    vec2 b = hash2(i + vec2(1.0, 0.0));
    vec2 c = hash2(i + vec2(0.0, 1.0));
    vec2 d = hash2(i + vec2(1.0, 1.0));

    // interpolant
    vec2 u = quintic_hermine(f);

    // noise
    float val = mix(mix(dot(a, f - vec2(0.0,0.0)),
                        dot(b, f - vec2(1.0,0.0)), u.x),
                    mix(dot(c, f - vec2(0.0,1.0)),
                        dot(d, f - vec2(1.0,1.0)), u.x), u.y);

    return val * 0.5 + 0.5;
}

void main()
{
    if (gl_InvocationID == 0)
    {
        vec4 vert = gl_in[gl_InvocationID].gl_Position;

        const vec3 l = vec3(sin(constants.time) * 5.0, 5.0, 0.0);
        float p = max(0.0, dot(vs_normal[gl_InvocationID], l));
        p += 1.0;

        float inner = noise(vert.xy * 0.2 + constants.time);
        float outer = noise(vert.zy * 0.2 + constants.time);
        inner = pow(inner, 4.0);
        outer = pow(outer, 4.0);
        inner *= 24.0;
        outer *= 24.0;

        inner = 1.0f;
        outer = 1.0f;
        gl_TessLevelInner[0] = inner;
        gl_TessLevelOuter[0] = inner;
        gl_TessLevelOuter[1] = outer;
        gl_TessLevelOuter[2] = outer;
    }

    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;

    // Pass attributes to tessellation evaluation shader
    tc_normal[gl_InvocationID] = vs_normal[gl_InvocationID];
    tc_texcoord[gl_InvocationID] = vs_texcoord[gl_InvocationID];
    tc_tess_inner[gl_InvocationID] = gl_TessLevelInner[0];
    tc_tess_outer[gl_InvocationID] = gl_TessLevelOuter[0];
}
