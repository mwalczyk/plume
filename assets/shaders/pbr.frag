#version 450
#extension GL_ARB_separate_shader_objects : enable

// Microfacet theory:
//
// The microfacet models attempts to model the tiny imperfections on the surface of an object.s
// Because each microfacet is generally much smaller than the size of a pixel, we statistically
// approximate the percentage of microfacets that are aligned with the halfway vector:
//
//                          H = (L + V) / ||L + V||
//
// We control this behavior with a "roughness" parameter.



// Specular and diffuse lighting:
//
// When light strikes a surface, it is either reflected (specular) or refracted (diffuse). In
// order for a lighting model to exhibit energy conservation, the outgoing light energy should
// never exceed the incoming light energy. Certain advanced techniques attempt to model subsurface
// scattering, where the refracted light is scattered and eventually re-exits the surface at
// a nearby point. For our purposes, we assume that all refracted light is absorbed.
//
// We generally divide materials into two categories: metallics, which immediately absorb all
// refracted light, and dielectrics which exhibit both reflection and refraction.
//
// To ensure energy conservation, we first calculate the fraction of light that is reflected.
// The fraction of light that is refracted then follows:
//
//                          float ks = calculate_specular_component(...);
//                          float kd = 1.0 - ks;
//
// Using this formulation, it is impossible for both the specular / diffuse contributions to
// exceed 1.0.



// Radiometry:
//
// Radiant flux is the transmitted energy of a light source, measured in Watts. We often make
// the simplifying assumption that radiant flux can be discretized into 3 channels: R, G, and B.
//
// The solid angle tells us the size or area of a shape projected onto the unit sphere. Think
// of being an observer at the center of this unit sphere and looking in the direction of the
// shape: the size of the observed silhouette is the solid angle.
//
// Radiant intensity measures the amount of radiant flux per unit solid angle. In other words,
// it measures the strength of a light source over a projected area onto the unit sphere.
//
// Radiance is the total observed energy over a differential area over the solid angle of a
// light. It is the measure of the amount of light in an area scaled by the incident angle
// of the light relative to the surface normal, cos(theta): the amount of incoming light
// energy is less at glancing angles.
//
// If we assume that the solid angle and observed area are infinitely small, we can use radiance
// to measure the flux of a single ray of light hitting a single point in space.
//
// In physically based shading, we generally care about the sum of all incoming light on a
// surface point. This is known as irradiance. So, we measure the contribution of the light
// sources in our scene not just from a single incoming light direction but from all light
// directions within a hemisphere centered around the point.



// BRDF:
//
// The bidirectional reflective distribution function takes the incoming light direction, the
// outgoing view direction, the surface normal, and a roughness parameter and approximates how
// much each individual light ray contributes to the final reflected light of an opaque surface
// given its material properties. For example, the BRDF of a perfect mirror-like surface would
// return 0.0 for every incoming light direction except the one whose (reflected) angle was the
// same as the outgoing view direction.
//
// The Cook-Torrance BRDF contains both a diffuse and specular part:
//
//                          fr = kd * f_lambert + ks * f_cook_torrance
//
// In the equation above, f_lambert is a constant factor, c / PI, where c is the albedo of the
// material. In contrast, f_cook_torrance is the specular component that is itself made up of
// three terms:
//    1.) Normal distribution function (D): the percentage of the microfacets that are aligned with
//        the halfway vector
//    2.) Geometric shadowing function (G): describes the self-shadowing property of the microfacets
//    3.) Fresnel factor (F): the strength of surface reflection at different surface angles

layout (location = 0) out vec4 o_color;

layout (location = 0) in vec3 vs_world_position;
layout (location = 1) in vec3 vs_color;
layout (location = 2) in vec3 vs_normal;
layout (location = 3) flat in uint vs_instance_id;

layout (std430, push_constant) uniform push_constants
{
	float time;
	float metallic;
} constants;

layout (set = 0, binding = 1) uniform sampler2D irradiance_map;

const float pi = 3.1415926535897932384626433832795;

// Cosine-based color palette generator from IQ: https://www.shadertoy.com/view/ll2GD3
vec3 palette(in float t,
						 in vec3 a,
						 in vec3 b,
						 in vec3 c,
						 in vec3 d)
{
    return a + b * cos(2.0 * pi * (c * t + d));
}

vec4 sample_equirectangular_map(in vec3 direction,
																sampler2D equirectangular)
{
	// From the OpenGL SuperBible, 6th Ed.
	vec2 uv;
	uv.y = direction.y;
	direction.x = normalize( direction.xz ).x * 0.5;
	float s = sign( direction.z ) * 0.5;
	uv.x = 0.75 - s * (0.5 - uv.x);
	uv.y = 0.5 + 0.5 * uv.y;

 	return texture(equirectangular, uv);
}

// Trowbridge-Reitz GGX normal distribution function
//
// Given the angle between the surface normal and the halfway vector and the surface roughness,
// compute the percentage of microfacets that are aligned with the halfway vector. This is a
// value between 0..1.
float normal_distribution(float n_dot_h,
                          float a)
{
  float a_squared = a * a;
  float denonimator = n_dot_h * n_dot_h * (a_squared - 1.0) + 1.0;
  denonimator = pi * denonimator * denonimator;

  return a_squared / denonimator;
}

// Smith's Schlick-GGX geometric shadowing function
//
// Given the surface normal, the view direction, and the surface roughness, compute the ratio
// of microfacets that are overshadowed by surrounding microfacets and thus, invisible to the
// viewer. Note that the geometric shadowing function is actually made up of two separate
// components: geometric obstruction (view direction) and geometric shadowing (light direction).
// This is a value between 0..1, where 0 represents complete microfacet shadowing and 1
// represents no microfacet shadowing.
float ggx(float n_dot_v,
          float k)
{
  float denonimator = n_dot_v * (1.0 - k) + k;

  return n_dot_v / denonimator;
}

float geometric_shadowing(float n_dot_v,
                          float n_dot_l,
                          float a)
{
  float k = ((a + 1.0) * (a + 1.0)) / 8.0; // Remapping of roughness parameter for direct lighting
  // k = (a * a) / 2.0;                    // Remapping of roughness parameter for IBL

  float ggx_0 = ggx(n_dot_v, k); // Geometric obstruction
  float ggx_1 = ggx(n_dot_l, k); // Geometric shadowing

  return ggx_0 * ggx_1;
}

// Fresnel-Schlick approximation
//
// Given the angle between the surface normal and the view direction and the base reflectivity of the,
// surface compute the percentage of light that is reflected. Every surface has a base reflectivity,
// and all surfaces become more reflective at glancing angles. The base reflectivity is calculated
// using the surface's index of refraction. For dielectrics, the base reflectivity is a grayscale
// value ~0.04. For metals, the base reflectivity is tinted and therefore represented as an RGB triple
// (reflectivity at normal incidence can vary per wavelength).
vec3 fresnel(in vec3 r0,
             float n_dot_v)
{
  return r0 + (1.0 - r0) * pow(1.0 - n_dot_v, 5.0);
}

struct light
{
	vec3 position;
	vec3 color;
};

void main()
{
	float t = constants.time;
	float r = constants.metallic;
	float pct = float(vs_instance_id) / 225.0;

	// For now, use a push constant to vary the roughness between 0..1
	float a = float(vs_instance_id + 1.0) / 225.0;

	// Note that 0: dielectric, 1: metal
	// Theoretically this should be a binary toggle, but most workflows allow the 'metallic' parameter
	// to vary smoothly between 0..1
	float metallic = constants.metallic;
	float ambient_occlusion = 1.0;
	vec3 albedo = palette(pct * 0.3, vec3(0.5,0.5,0.5), vec3(0.5,0.5,0.5), vec3(1.0,1.0,1.0), vec3(0.0,0.10,0.20));
											 //vec3(0.5,0.5,0.5), vec3(0.5,0.5,0.5), vec3(1.0,1.0,1.0), vec3(0.3,0.20,0.20));
											 //vec3(0.5,0.5,0.5), vec3(0.5,0.5,0.5), vec3(1.0,1.0,1.0), vec3(0.0,0.33,0.67));
	// Determine the base reflectivity of our material based on the 'metallic' parameter the Fresenel term
	vec3 r0 = vec3(0.04);
	r0 = mix(r0, albedo, metallic);

	// The world-space position of the camera (this should eventually be passed in as a uniform)
	const vec3 camera_position = vec3(0.0, 0.0, 40.0);

	// Setup scene lighting
	const uint number_of_lights = 4;
	const float grid = 4.0;
	const float z = 8.0;
	light scene_lights[] = light[4](
		light(vec3(sin(constants.time) * grid, -grid, z), vec3(23.47, 21.31, 20.79)),
		light(vec3(cos(constants.time) * -grid, -grid, z), vec3(23.47, 21.31, 20.79)),
		light(vec3(sin(constants.time) * -grid, grid, z), vec3(23.47, 21.31, 20.79)),
		light(vec3(cos(constants.time) * grid, grid, z), vec3(23.47, 21.31, 20.79))
	);

	// Calculate direct lighting
	vec3 outgoing_radiance = vec3(0.0);

	for (int i = 0; i < number_of_lights; ++i)
	{
		// Calculate the observed radiance coming from the current light source at the fragment's position
		float distance_to_light = length(scene_lights[i].position - vs_world_position);
		float distance_attenuation = 1.0 / (distance_to_light * distance_to_light);
		vec3 radiance = scene_lights[i].color * distance_attenuation;

		// Calculate the relevant direction vectors used throughout
		vec3 n = normalize(vs_normal);
		vec3 l = normalize(scene_lights[i].position - vs_world_position);
		vec3 v = normalize(camera_position - vs_world_position);
		vec3 h = normalize(l + v);

		// Compute relevant dot products
		float n_dot_h = max(dot(n, h), 0.0);
		float n_dot_v = max(dot(n, v), 0.0);
		float n_dot_l = max(dot(n, l), 0.0);
		float h_dot_v = max(dot(h, v), 0.0);

		// Calculate the terms for the Cook-Torrance BRDF
		float D = normal_distribution(n_dot_h, a);
		float G = geometric_shadowing(n_dot_v, n_dot_l, a);
		vec3 F = fresnel(r0, n_dot_v);

		// Combine terms, taking care to prevent a division by zero by adding a small epsilon term to
		// the denonimator
		vec3 cook_torrance = (D * G * F) / (4.0 * n_dot_v * n_dot_l + 0.001);

		// Determine the ratio of specular to diffuse contributions from the current light source.
		// Note that the Fresnel-Schlick approximation gives us the exact percentage of reflected
		// (specular) light at the fragment's position. So, the diffuse contribution from this light
		// source must be (1 - F).
		vec3 ks = F;
		vec3 kd = vec3(1.0) - ks;

		// If the material is a metal, it should not have a diffuse component (all refracted light
		// is immediately absorbed)
		kd *= 1.0 - metallic;

		// The full reflectance equation is below. Note that we leave ks out of this equation. Since
		// ks = F, this is already accounted for in the Cook-Torrance BRDF, so we don't want to multiply
		// by ks twice.
		outgoing_radiance += (kd * (albedo / pi) + cook_torrance) * radiance * n_dot_l;
	}

	// Add a (somewhat) arbitrary ambient term to the direct lighting result
	//const vec3 ambient = vec3(0.03) * albedo * ambient_occlusion;

	// If we are using IBL, we calculate the indirect lighting term as follows
	vec3 n = normalize(vs_normal);
	vec3 v = normalize(camera_position - vs_world_position);
	float n_dot_v = max(dot(n, v), 0.0);
	vec3 ks = fresnel(r0, n_dot_v);
	vec3 kd = 1.0 - ks;
	kd *= 1.0 - metallic;
	vec3 irradiance = sample_equirectangular_map(n, irradiance_map).rgb;	// irradiance
	vec3 ambient = irradiance * albedo * kd * ambient_occlusion;

	// Add the ambient term
	outgoing_radiance += ambient;

	// Apply tone mapping then gamma correction
	outgoing_radiance /= outgoing_radiance + vec3(1.0);
	//outgoing_radiance = vec3(1.0) - exp(-outgoing_radiance * 2.0);
	outgoing_radiance = pow(outgoing_radiance, vec3(1.0 / 2.2));

	vec4 samp = texture(irradiance_map, gl_FragCoord.xy / vec2(800.0));

  o_color = vec4(outgoing_radiance, 1.0);
}
