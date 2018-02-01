#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (set = 0, binding = 1) uniform sampler3D sfd_map;

// Inputs
layout (location = 0) in vec3 vs_normal;
layout (location = 1) in vec2 vs_texcoord;

// Outputs
layout (location = 0) out vec4 o_color;

layout(std430, push_constant) uniform push_constants
{
	float time;
	vec2 mouse;
} constants;

const float pi = 3.141592653589793;
const uint MAX_STEPS = 128u;
const float MAX_TRACE_DISTANCE = 32.0;
const float MIN_HIT_DISTANCE = 0.0001;

/****************************************************
 *
 * structs
 *
 */
struct ray
{
	vec3 o;
	vec3 d;
};

/****************************************************
 *
 * utility functions
 *
 */
vec3 hammersley(in uint i, in uint num_samples)
{
	float u = float(i) / float(num_samples);
	float v = float(bitfieldReverse(i)) * 2.3283064365386963e-10;
	float phi = v * 2.0 * pi;
    float cos_theta = 1.0 - u;
    float sin_theta = sqrt(1.0 - cos_theta * cos_theta);

    return vec3(cos(phi) * sin_theta, sin(phi) * sin_theta, cos_theta);
}

vec3 palette(in float t,
			 in vec3 a,
			 in vec3 b,
			 in vec3 c,
			 in vec3 d)
{
	// cosine-based color palette generator from IQ
    return a + b * cos(2.0 * pi * (c * t + d));
}

float hash(float n)
{
    return fract(sin(n) * 1e4);
}

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

#define NUM_OCTAVES 7

float noise(in vec3 x)
{
	const vec3 step = vec3(110.0, 241.0, 171.0);
	vec3 i = floor(x);
	vec3 f = fract(x);

	// For performance, compute the base input to a 1D hash from the integer part of the argument and the
	// incremental change to the 1D based on the 3D -> 1D wrapping
    float n = dot(i, step);

	vec3 u = f * f * (3.0 - 2.0 * f);
	return mix(mix(mix( hash(n + dot(step, vec3(0, 0, 0))), hash(n + dot(step, vec3(1, 0, 0))), u.x),
                   mix( hash(n + dot(step, vec3(0, 1, 0))), hash(n + dot(step, vec3(1, 1, 0))), u.x), u.y),
               mix(mix( hash(n + dot(step, vec3(0, 0, 1))), hash(n + dot(step, vec3(1, 0, 1))), u.x),
                   mix( hash(n + dot(step, vec3(0, 1, 1))), hash(n + dot(step, vec3(1, 1, 1))), u.x), u.y), u.z);
}

float fbm(in vec3 x)
{
	float v = 0.0;
	float a = 0.5;
	vec3 shift = vec3(100.0);
	for (int i = 0; i < NUM_OCTAVES; ++i)
    {
		v += a * noise(x);
		x = x * 2.0 + shift;
		a *= 0.5;
	}

	return v;
}

mat2 rotate_2d(float d)
{
	return mat2(cos(d), -sin(d), sin(d), cos(d));
}

mat3 lookat(in vec3 t, in vec3 p)
{
	vec3 k = normalize(t - p);
	vec3 i = cross(k, vec3(0.0, 1.0, 0.0));
	vec3 j = cross(i, k);

	return mat3(i, j, k);
}

/****************************************************
 *
 * modifiers
 *
 */
vec3 modifier_repeat(in vec3 p, in vec3 r)
{
	return mod(p, r) - r * 0.5;
}

float modifier_polar(inout vec2 p, float repetitions)
{
	// From HG_SDF
	float angle = 2.0 * pi / repetitions;
	float a = atan(p.y, p.x) + angle / 2.0;
	float r = length(p);
	float c = floor(a / angle);
	a = mod(a, angle) - angle / 2.0;
	p = vec2(cos(a), sin(a)) * r;

	if (abs(c) >= (repetitions / 2.0)) c = abs(c);

	return c;
}

/****************************************************
 *
 * operators
 *
 */
float op_union(float a, float b)
{
	return min(a, b);
}

float op_intersect(float a, float b)
{
	return max(a, b);
}

float op_smin(float a, float b, float k)
{
    float h = clamp(0.5 + 0.5 * (b - a) / k, 0.0, 1.0);
    return mix(b, a, h) - k * h * (1.0 - h);
}

/****************************************************
 *
 * primitives
 *
 */
float sdf_sphere(in vec3 p, in vec3 center, float radius)
{
	return length(center - p) - radius;
}

float sdf_box(in vec3 p, in vec3 b)
{
  vec3 d = abs(p) - b;
  return min(max(d.x, max(d.y, d.z)), 0.0) + length(max(d, 0.0));
}

float sdf_plane(in vec3 p, in float h)
{
	return p.y - h;
}

#define PHI (sqrt(5) * 0.5 + 0.5)

float sdf_blob(in vec3 p) {
	p = abs(p);
	if (p.x < max(p.y, p.z)) p = p.yzx;
	if (p.x < max(p.y, p.z)) p = p.yzx;
	float b = max(max(max(dot(p, normalize(vec3(1, 1, 1))),
						  dot(p.xz, normalize(vec2(PHI+1, 1)))),
					  dot(p.yx, normalize(vec2(1, PHI)))),
				  dot(p.xz, normalize(vec2(1, PHI))));

	float l = length(p);
	return l - 1.5 - 0.2 * (1.5 / 2) * cos(min(sqrt(1.01 - b / l) * (pi / 0.25),
										   pi)); 
}

/****************************************************
 *
 * ray marching utilities
 *
 */
vec2 map(in vec3 p)
{
	float t = constants.time;
	float s = sin(t);
	float c = cos(t);
    vec2 m = constants.mouse;

	// Displacers
	float freq = m.x;
	float ampl = m.y * 2.0;
    vec3 d = p + (fbm(p * freq + t) * 2.0 - 1.0) * ampl;

    // Shapes
    float sphere = sdf_sphere(d, vec3(0.0), 5.0);
    float box = sdf_box(p, vec3(5.0));
    float blob = sdf_blob(p * 0.5);

    const float id = 0.0;

	return vec2(id, blob);
}

vec3 calculate_normal(in vec3 p)
{
    const vec3 e = vec3(0.001, 0.0, 0.0);
    vec3 n = vec3(map(p + e.xyy).y - map(p - e.xyy).y,	// Gradient x
                  map(p + e.yxy).y - map(p - e.yxy).y,	// Gradient y
                  map(p + e.yyx).y - map(p - e.yyx).y); // Gradient z

    return normalize(n);
}

vec2 raymarch(in ray r)
{
	float current_total_distance = 0.0;
	float current_id = -1.0;

	for (uint i = 0u; i < MAX_STEPS; ++i)
	{
        // Step along the ray
		vec3 p = r.o + current_total_distance * r.d;

        // Here, `hit_info` is a `vec2` that contains:
        // 1) The ID of the object that was hit
        // 2) The distance from `p` to the object in question
		vec2 hit_info = map(p);
		float id = hit_info.x;
		float dist = hit_info.y;

		current_total_distance += dist;

		if (dist < MIN_HIT_DISTANCE)
		{
			current_id = id;
			break;
		}

		if(current_total_distance > MAX_TRACE_DISTANCE)
		{
			current_total_distance = 0.0;
			break;
		}
	}

	return vec2(current_id, current_total_distance);
}

float ambient_occlusion(in vec3 p, in vec3 n)
{
	float attenuation = 0.75;
	float ao;
    float accum = 0.0;
    float scale = 1.0;
    for(int step = 0; step < 5; step++)
    {
    	float hr = 0.01 + 0.02 * float(step * step);
        vec3 aopos = n * hr + p + 0.001;

        float dist = map(aopos).y;
        ao = -(dist - hr);

        accum += ao * scale;

        scale *= attenuation;
    }
	ao = 1.0 - clamp(accum, 0.0, 1.0);

	return ao;
}

void main()
{
	vec2 uv = vs_texcoord * 2.0 - 1.0;
    float t = constants.time;
	vec2 m = constants.mouse;
	float s = sin(t * 0.25);
	float c = cos(t * 0.25);
	float orbit = 8.0;

	vec3 camera_position = vec3(s * orbit, 10.0, c * orbit);
	mat3 lookat = lookat(vec3(0.0), camera_position);

	// Construct the primary ray
	vec3 ro = camera_position;
	vec3 rd = normalize(lookat * vec3(uv.xy, 1.0));
	ray r = ray(ro, rd);

	// Raymarch
	vec2 res = raymarch(r);
	vec3 hit = ro + rd * res.y;

	// Per-object settings
    const vec3 background = vec3(0.0);
	vec3 color = vec3(0.0);
	switch(int(res.x))
	{
		case 0:
            // Diffuse lighting
			vec3 n = calculate_normal(hit);
			vec3 l = normalize(vec3(1.0, 5.0, 0.0));
			float d = max(0.1, dot(n, l));

            // Ambient occlusion
            float ao = ambient_occlusion(hit, n);

            vec3 nc = n * 0.5 + 0.5;
            color = vec3(d * ao * nc);
			break;
		case 1: 
			// Placeholder
			break; 
		case 2: 
			// Placeholder
			break;
			// etc...
		default:
			color = background;
			break;
	}

	o_color = vec4(color, 1.0);
}
