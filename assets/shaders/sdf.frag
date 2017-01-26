#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (set = 0, binding = 1) uniform sampler2D CombinedImageSampler;

layout(location = 0) in vec2 vsTexcoord;

layout(location = 0) out vec4 oColor;

layout(std430, push_constant) uniform PushConstants
{
	float time;
	vec2 mouse;
} constants;

const int MAX_STEPS = 128;
const float MAX_TRACE_DISTANCE = 20.0;
const float MIN_HIT_DISTANCE = 0.01;

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

vec3 modifier_repeat(in vec3 q, in vec3 r)
{
	return mod(q, r) - r * 0.5;
}

float op_union(float d0, float d1)
{
	return min(d0, d1);
}

float op_smin(float a, float b, float k)
{
    a = pow(a, k);
    b = pow(b, k);
    return pow((a * b) / (a + b), 1.0 / k);
}

float sdf_sphere(in sphere s, in vec3 p)
{
	return length(s.center - p) - s.radius;
}

float t = constants.time;
sphere s0 = sphere(10.0, vec3(0.0));

float map(in vec3 p)
{
	float s = sin(t) * 0.5 + 0.5;
	float c = cos(t) * 0.5 + 0.5;
	float n0 = noise((p.xz + p.zy) * 0.25 + t) * 2.0 - 1.0;
	float n1 = noise(p.yz * n0 * 0.5 + t) * 2.0 - 1.0;

	p += normalize(p) * n0 * n1 * 1.0;
	p += sin(p.x + t) * sin(p.y + t) * sin(p.z + t) * 1.0;
	float dist = sdf_sphere(s0, p);

	return dist;
}

vec3 calculate_normal(in vec3 p)
{
    vec3 eps = vec3(0.001, 0.0, 0.0);
    vec3 n = vec3(map(p + eps.xyy) - map(p - eps.xyy),
                  map(p + eps.yxy) - map(p - eps.yxy),
                  map(p + eps.yyx) - map(p - eps.yyx));
    return normalize(n);
}

vec3 raymarch(in ray r)
{
	vec3 color = vec3(0.0);
	vec3 current_position = r.origin;

	for (int i = 0; i < MAX_STEPS; ++i)
	{
		float distance_to = map(current_position);

		if (distance_to < MIN_HIT_DISTANCE)
		{
			const vec3 light_position = vec3(0.0, 14.0, 14.0);

			vec3 to_light = normalize(current_position - light_position);
			vec3 normal = calculate_normal(current_position);

			float lighting = max(0.4, dot(normal, to_light));
			color = normal * 0.5 + 0.5;
			color = 1.0 - color;
			color *= pow(1.0 - lighting, 0.5);

			break;
		}
		current_position += distance_to * r.direction;
	}
	return color;
}

void main()
{
	vec4 sampledColor = texture(CombinedImageSampler, vsTexcoord);
	float mx = constants.mouse.x * 2.0 - 1.0;

	// remap to -1..1
	vec2 uv = vsTexcoord * 2.0 - 1.0;
	vec3 camera_position = vec3(0.0, 0.0, -20.0);
	vec3 target = vec3(0.0, 0.0, 0.0);

	// construct the camera's lookat matrix
	vec3 forward = normalize(target - camera_position);
	vec3 right = cross(forward, vec3(0.0, 1.0, 0.0));
	vec3 up = cross(right, forward);

	mat3 lookat = mat3(right, up, forward);

	vec3 ro = camera_position;
	vec3 rd = vec3(uv.xy, 1.0);
	rd = normalize(lookat * rd);

	ray r = ray(ro, rd);

	vec3 result = raymarch(r);

	vec4 color = vec4(result, 1.0);

	oColor = color;
}
