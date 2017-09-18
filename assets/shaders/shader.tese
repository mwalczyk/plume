#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (triangles) in;

layout(std430, push_constant) uniform push_constants
{
	float time;
	vec2 mouse;
} constants;

layout (set = 0, binding = 0) uniform uniform_buffer_object
{
	mat4 model;
	mat4 view;
	mat4 projection;
} ubo;

// Inputs
layout (location = 0) in vec3 tc_normal[];
layout (location = 1) in vec2 tc_texcoord[];
layout (location = 2) in float tc_tess_inner[];
layout (location = 3) in float tc_tess_outer[];

// Outputs
layout (location = 0) out vec3 te_normal;
layout (location = 1) out vec2 te_texcoord;
layout (location = 2) out float te_noise;

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
    gl_Position = (gl_TessCoord.x * gl_in[0].gl_Position) +
                  (gl_TessCoord.y * gl_in[1].gl_Position) +
                  (gl_TessCoord.z * gl_in[2].gl_Position);

	// Pass to fragment shader
	te_normal = gl_TessCoord.x * tc_normal[0] +
				gl_TessCoord.y * tc_normal[1] +
				gl_TessCoord.z * tc_normal[2];

	te_texcoord = gl_TessCoord.x * tc_texcoord[0] + gl_TessCoord.y * tc_texcoord[1] + gl_TessCoord.z * tc_texcoord[2];

    // Displacement along normal
    float a = (tc_tess_inner[0] + tc_tess_outer[0]) / 64.0;
    vec2 s = gl_Position.zy * 1.0 + constants.time;
    te_noise = noise(s * 0.25) * a;

    gl_Position.xyz -=  te_normal * te_noise;

    gl_Position = ubo.projection * ubo.view * ubo.model * gl_Position;
}
