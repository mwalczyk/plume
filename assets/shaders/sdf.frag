#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 vsColor;
layout(location = 1) in vec2 vsTexcoord;

layout(location = 0) out vec4 oColor;

layout(std430, push_constant) uniform PushConstants
{
	float time;
	vec2 mouse;
	vec3 position;
} constants;

const int MAX_STEPS = 64;
const float MIN_HIT_DISTANCE = 0.001;

struct sphere
{
	float radius;
	vec3 center;
};

struct ray
{
	vec3 origin;
	vec3 direction;
};

sphere s = sphere(1.0, vec3(0.0, 0.0, 10.0));

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


float sdf_sphere(in sphere s, in vec3 p)
{
	s.radius += noise(p.xy * 10.0 + constants.time) * 0.1;
	return length(s.center - p) - s.radius;
}

vec3 raymarch(in ray r)
{
	vec3 color = vec3(0.0);
	vec3 start = r.origin;

	for (int i = 0; i < MAX_STEPS; ++i)
	{
		float distance_to = sdf_sphere(s, start);
		if (distance_to < MIN_HIT_DISTANCE)
		{
			vec3 light_position = vec3(4.0, 5.0, 0.0);
			vec3 to_light = normalize(start - light_position);

			vec3 normal = normalize(s.center - start);
			float lighting = max(0.25, dot(normal, to_light));

			color = (normal * 0.5 + 0.5) * lighting;
			break;
		}
		start += distance_to * r.direction;
	}
	return color;
}

void main()
{
	vec2 uv = vsTexcoord * 2.0 - 1.0;

	vec3 camera_position = vec3(0.0, 0.0, -10.0);
	vec3 camera_up = vec3(0.0, 1.0, 0.0);
	vec3 camera_right = vec3(1.0, 0.0, 0.0);

	vec3 ro = camera_position;
	vec3 rd = normalize(vec3(uv, 0.0) - ro);
	ray r = ray(ro, rd);

	vec3 result = raymarch(r);
	vec2 m = constants.mouse;
	vec4 color = vec4(result, 1.0);

	oColor = color;
}
