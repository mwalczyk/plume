#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (set = 0, binding = 0) uniform UniformBufferObject
{
	mat4 model;
	mat4 view;
	mat4 projection;
} ubo;

layout(std430, push_constant) uniform PushConstants
{
	float time;
	vec2 mouse;
} constants;

// Vertex shader inputs
layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inColor;

// Vertex shader outputs
out gl_PerVertex
{
	vec4 gl_Position;
};

layout (location = 0) out vec3 vsColor;

vec2 hash2(in vec2 p)
{
    p = vec2(dot(p, vec2(12.9898, 78.233)),
             dot(p, vec2(139.234, 98.187)));

    return -1.0 + 2.0 * fract(sin(p) * 43758.5453123);
}

vec2 quinticHermine(in vec2 x)
{
    return x * x * x * (x * (x * 6.0 - 15.0) + 10.0);
}

float noise(in vec2 p)
{
    vec2 i = floor(p);
    vec2 f = fract(p);

    vec2 a = hash2(i + vec2(0.0, 0.0));
    vec2 b = hash2(i + vec2(1.0, 0.0));
    vec2 c = hash2(i + vec2(0.0, 1.0));
    vec2 d = hash2(i + vec2(1.0, 1.0));
    vec2 u = quinticHermine(f);

    float val = mix(mix(dot(a, f - vec2(0.0,0.0)),
                        dot(b, f - vec2(1.0,0.0)), u.x),
                    mix(dot(c, f - vec2(0.0,1.0)),
                        dot(d, f - vec2(1.0,1.0)), u.x), u.y);
    return val * 0.5 + 0.5;
}

void main()
{
	vsColor = inColor;

	float n = noise(inPosition.xy * 2.0 + constants.time) * 2.0 - 1.0;
	vec3 normalDirection = normalize(inPosition);
	vec3 modifiedPosition = inPosition + normalDirection * n * constants.mouse.x;

  gl_Position = ubo.projection * ubo.view * ubo.model * vec4(modifiedPosition, 1.0);
}
