#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (set = 0, binding = 1) uniform sampler3D sfd_map;

// Inputs
layout (location = 0) in vec3 te_normal;
layout (location = 1) in vec2 te_texcoord;
layout (location = 2) in float te_noise;

// Outputs
layout (location = 0) out vec4 o_color;

layout(std430, push_constant) uniform push_constants
{
	float time;
	vec2 mouse;
} constants;

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
	float t = constants.time;
	vec2 m = constants.mouse;

    //vec3 s = texture(sfd_map, vec3(te_texcoord, 0.0)).rgb;

    float p = pow(te_noise, 0.025);
	o_color = vec4(vec3(p), 1.0);
}

// const float pi = 3.141592653589793;
// const uint MAX_STEPS = 128u;
// const float MAX_TRACE_DISTANCE = 32.0;
// const float MIN_HIT_DISTANCE = 0.01;
//
// /****************************************************
//  *
//  * structs
//  *
//  */
// struct ray
// {
// 	vec3 o;
// 	vec3 d;
// };
//
// /****************************************************
//  *
//  * utility functions
//  *
//  */
// vec3 hammersley(in uint i, in uint num_samples)
// {
// 	float u = float(i) / float(num_samples);
// 	float v = float(bitfieldReverse(i)) * 2.3283064365386963e-10;
//
// 	float phi = v * 2.0 * pi;
//     float cosTheta = 1.0 - u;
//     float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
//     return vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
// }
//
// const vec3[] preset =
// {
//     vec3(0.2,0.2,0.7),
//     vec3(0.5,0.5,0.5),
//     vec3(1.0,0.7,0.4),
//     vec3(0.0,0.15,0.20)
// };
//
// vec3 palette(in float t,
// 			 in vec3 a,
// 			 in vec3 b,
// 			 in vec3 c,
// 			 in vec3 d)
// {
// 	// cosine-based color palette generator from IQ
//     return a + b * cos(2.0 * pi * (c * t + d));
// }
//
// vec2 hash2(in vec2 p)
// {
//     p = vec2(dot(p, vec2(12.9898, 78.233)),
//              dot(p, vec2(139.234, 98.187)));
//
//     return fract(sin(p) * 43758.5453123) * 2.0 - 1.0;
// }
//
// vec2 quintic_hermine(in vec2 x)
// {
//     return x * x * x * (x * (x * 6.0 - 15.0) + 10.0);
// }
//
// float noise(in vec2 p)
// {
//     vec2 i = floor(p);
//     vec2 f = fract(p);
//
//     // four corners
//     vec2 a = hash2(i + vec2(0.0, 0.0));
//     vec2 b = hash2(i + vec2(1.0, 0.0));
//     vec2 c = hash2(i + vec2(0.0, 1.0));
//     vec2 d = hash2(i + vec2(1.0, 1.0));
//
//     // interpolant
//     vec2 u = quintic_hermine(f);
//
//     // noise
//     float val = mix(mix(dot(a, f - vec2(0.0,0.0)),
//                         dot(b, f - vec2(1.0,0.0)), u.x),
//                     mix(dot(c, f - vec2(0.0,1.0)),
//                         dot(d, f - vec2(1.0,1.0)), u.x), u.y);
//
//     return val * 0.5 + 0.5;
// }
//
// mat2 rotate_2d(float d)
// {
// 	return mat2(cos(d), -sin(d), sin(d), cos(d));
// }
//
// mat3 lookat(in vec3 t, in vec3 p)
// {
// 	vec3 k = normalize(t - p);
// 	vec3 i = cross(k, vec3(0.0, 1.0, 0.0));
// 	vec3 j = cross(i, k);
//
// 	return mat3(i, j, k);
// }
//
// /****************************************************
//  *
//  * modifiers
//  *
//  */
// vec3 modifier_repeat(in vec3 p, in vec3 r)
// {
// 	return mod(p, r) - r * 0.5;
// }
//
// float modifier_polar(inout vec2 p, float repetitions)
// {
// 	// From HG_SDF
// 	float angle = 2.0 * pi / repetitions;
// 	float a = atan(p.y, p.x) + angle / 2.0;
// 	float r = length(p);
// 	float c = floor(a / angle);
// 	a = mod(a, angle) - angle / 2.0;
// 	p = vec2(cos(a), sin(a)) * r;
//
// 	if (abs(c) >= (repetitions / 2.0)) c = abs(c);
//
// 	return c;
// }
//
// /****************************************************
//  *
//  * operators
//  *
//  */
// float op_union(float a, float b)
// {
// 	return min(a, b);
// }
//
// float op_intersect(float a, float b)
// {
// 	return max(a, b);
// }
//
// float op_smin(float a, float b, float k)
// {
//     float h = clamp(0.5 + 0.5 * (b - a) / k, 0.0, 1.0);
//     return mix(b, a, h) - k * h * (1.0 - h);
// }
//
// /****************************************************
//  *
//  * primitives
//  *
//  */
// float sdf_sphere(in vec3 p, in vec3 center, float radius)
// {
// 	return length(center - p) - radius;
// }
//
// float sdf_box(in vec3 p, in vec3 b)
// {
//   vec3 d = abs(p) - b;
//   return min(max(d.x, max(d.y, d.z)), 0.0) + length(max(d, 0.0));
// }
//
// float sdf_plane(in vec3 p, in float h)
// {
// 	return p.y - h;
// }
//
// /****************************************************
//  *
//  * ray marching utilities
//  *
//  */
// vec2 map(in vec3 p)
// {
// 	float t = constants.time;
// 	float s = sin(t);
// 	float c = cos(t);
//     float md = s;
//
// 	// Parameters
// 	const float grid_density = 1.225;
// 	const float displacement = 1.55;
// 	const float radius_decay = 1.95;
// 	float attraction = 1.0;//noise(p.yz) * 0.5 + 0.5;
// 	attraction *= 1.0;
//
// 	// Displacers
// 	const float noise_freq = 0.75;
// 	vec3 displaced = p + sin(p.x * noise_freq + t) *
// 					     cos(p.y * noise_freq + t) *
// 					     sin(p.z * noise_freq + t) *
// 					     displacement;
//     displaced += noise(vec2(length(p)) + t);
//
// 	vec3 cp = p * grid_density;
// 	vec2 n = floor(cp.xz);
// 	vec2 f = fract(cp.xz);
// 	float d = 10.0;
// 	for (int j = -1; j <= 1; ++j)
// 	{
// 		for (int i = -1; i <= 1; ++i)
// 		{
// 			mat2 rot = rotate_2d(t);
// 			vec2 g = vec2(float(i), float(j));
// 			vec2 o = hash2(n + g) * 0.5 + 0.5;
// 			vec2 r = g - f + o;
//
//  			float radius = pow(0.25, length(cp)) * radius_decay;
// 			float k = sdf_sphere(vec3(r.x, cp.y, r.y), vec3(0.0, o.x * 16.0 , 0.0), radius);
//
// 			if (k < d)
// 			{
// 				d = k;
// 			}
// 		}
// 	}
//
// 	// Add the large sphere in the center
//     //float sphere = sdf_sphere(displaced, vec3(0.0, c, 0.0), 8.5);
//     //float aabb = max(sdf_box(p, vec3(8.0)), sphere);
//
//     float aabb = sdf_box(displaced, vec3(6.0));
//     float comb = op_smin(aabb, d, attraction);
//     float clip = max(comb, sdf_sphere(p, vec3(0.0, 0.0, 0.0), 7.0));
//
// 	float id = 0.0;
//
// 	return vec2(id, clip);
// }
//
// vec3 calculate_normal(in vec3 p)
// {
//     const vec3 e = vec3(0.001, 0.0, 0.0);
//
//     vec3 n = vec3(map(p + e.xyy).y - map(p - e.xyy).y,	// gradient x
//                   map(p + e.yxy).y - map(p - e.yxy).y,	// gradient y
//                   map(p + e.yyx).y - map(p - e.yyx).y); // gradient z
//
//     return normalize(n);
// }
//
// vec2 raymarch(in ray r)
// {
// 	float current_total_distance = 0.0;
// 	float current_id = -1.0;
//
// 	for (uint i = 0u; i < MAX_STEPS; ++i)
// 	{
// 		vec3 p = r.o + current_total_distance * r.d;
//
// 		vec2 hit_info = map(p);
// 		float id = hit_info.x;
// 		float dist = hit_info.y;
//
// 		current_total_distance += dist;
//
// 		if (dist < MIN_HIT_DISTANCE)
// 		{
// 			current_id = id;
// 			break;
// 		}
//
// 		if(current_total_distance > MAX_TRACE_DISTANCE)
// 		{
// 			current_total_distance = 0.0;
// 			break;
// 		}
// 	}
//
// 	return vec2(current_id, current_total_distance);
// }
//
// void main()
// {
// 	vec2 uv = vs_texcoord * 2.0 - 1.0;
//
//     float t = constants.time;
// 	vec2 m = constants.mouse;
//
// 	float s = sin(t * 0.5);
// 	float c = cos(t * 0.5);
// 	float orbit = 15.0;
//
//         float ipos = floor(t);
//     	float fpos = fract(t);
//     	fpos = pow(fpos, 10.0);
//     	vec3 sphere_pos_prev = hammersley(int(ipos), 256);
//     	vec3 sphere_pos_next = hammersley(int(ipos + 1.0), 256);
//     	vec3 current_samp = mix(sphere_pos_prev, sphere_pos_next, fpos) * 35.0;
//
// 	vec3 camera_position = vec3(s * orbit, 15.0, c * orbit);
// 	mat3 lookat = lookat(vec3(0.0), camera_position);
//
// 	// Construct the primary ray
// 	vec3 ro = camera_position;
// 	vec3 rd = normalize(lookat * vec3(uv.xy, 1.0));
// 	ray r = ray(ro, rd);
//
// 	// Raymarch
// 	vec2 res = raymarch(r);
// 	vec3 hit = ro + rd * res.y;
//
// 	const vec3 background = vec3(0.0);
// 	vec3 color = vec3(0.0);
//
// 	// per-object settings
// 	switch(int(res.x))
// 	{
// 		case 0:
// 			float seed = res.y * 0.1325;
// 			vec3 n = calculate_normal(hit);
// 			vec3 l = normalize(vec3(0.0, 10.0, -2.0));
//
// 			float d = max(0.05, dot(n, l));
// 			color = vec3(pow(d, 2.25));
// 		//	color *= palette(dot(n, n) * 18.0, preset[0], preset[1], preset[2], preset[3]);
//
// 			break;
// 		case 1: break;
// 		case 2: break;
// 		default:
// 			color = background;
// 			break;
// 	}
//
// 	o_color = vec4(color, 1.0);
// }
