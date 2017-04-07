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

layout(std430, push_constant) uniform push_constants
{
	float time;
	float roughness;
} constants;

const float pi = 3.1415926535897932384626433832795;

// Trowbridge-Reitz GGX normal distribution function
//
// Given the angle between the surface normal and the halfway vector and the surface roughness,
// compute the percentage of microfacets that are aligned with the halfway vector. This is a
// value between 0..1.
float normal_distribution(float n_dot_h,
                          float a)
{
  // Ignore cases where the cosine is negative
  n_dot_h = max(n_dot_h, 0.0);

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

  float ggx_0 = ggx(max(n_dot_v, 0.0), k); // Geometric obstruction
  float ggx_1 = ggx(max(n_dot_l, 0.0), k); // Geometric shadowing

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


void main()
{
  const vec3 light = vec3(3.0, 3.0, 2.2);

  vec3 n = normalize(vs_world_position);
  vec3 l = normalize(vs_world_position - light);
  vec3 v = normalize(vec3(0.0, 0.0, 1.0) - vs_world_position);
  vec3 h = normalize(l + v);

  // Determine the amount of specular and diffuse contributions
  float ks = 0.0; //calculate_specular_component(...);
  float kd = 1.0 - ks;
  float t = constants.time;

  // For now, we have a red surface
  vec3 albedo = vec3(1.0, 0.0, 0.0);

  // Compute relevant dot products
  float n_dot_h = dot(n, h);
  float n_dot_v = dot(n, v);
  float n_dot_l = dot(n, l);

  // 0: dielectric, 1: metal
  // Theoretically this should be a binary toggle, but most workflows allow the 'metallic' parameter
  // to vary smoothly between 0..1
  float metallic = 0.0;

  // Determine the base reflectivity of our material based on the 'metallic' parameter
  vec3 r0 = vec3(0.04);
  r0 = mix(r0, albedo, metallic);

	// Calculate
	float a = constants.roughness;
	float D = normal_distribution(n_dot_h, a);

	// For now, just calculate a Lambertian diffuse term
  float light_intensity = max(dot(n, l), 0.1);
	vec3 lambertian = albedo * light_intensity;

  o_color = vec4(vec3(D), 1.0);
}
