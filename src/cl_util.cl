#pragma once
#include "cl_def.cl"

float degrees_to_radians(float degrees) {
	return degrees * pi / 180.0f;
}

// https://stackoverflow.com/a/16077942
float random_float(uint2* seed) {
#ifdef OPENCL
	const float invMaxInt = 1.0f/4294967296.0f;

	uint x = (*seed).x * 17 + (*seed).y * 13123;
	(*seed).x = (x<<13) ^ x;
	(*seed).y ^= (x<<7);

	uint tmp =  
		(x * (x * x * 15731 + 74323) + 871483) 
		+ (x * (x * x * 13734 + 37828) + 234234) 
		+ (x * (x * x * 11687 + 26461) + 137589);

	return convert_float(tmp) * invMaxInt;
#endif
	// TODO: Host side code if needed
}

float random_float_ranged(uint2* seed1, float min, float max) {
	return min + (max-min) * random_float(seed1);
}

float3 random_float3(uint2* seed) {
	return (float3){random_float(seed), random_float(seed), random_float(seed)};
}

float3 random_float3_ranged(uint2* seed, float min, float max) {
	return (float3){
		random_float_ranged(seed, min, max), 
		random_float_ranged(seed, min, max), 
		random_float_ranged(seed, min, max)
	};
}

float3 random_in_unit_sphere(uint2* seed) {
	while(true) {
		float3 p = random_float3_ranged(seed, -1, 1);
		if(length(p) * length(p) >= 1) continue;

		return p;
	}
}

float3 random_unit_vector(uint2* seed) {
	return normalize(random_in_unit_sphere(seed));
}

float3 random_in_hemisphere(float3 normal, uint2* seed) {
	float3 in_unit_sphere = random_in_unit_sphere(seed);

	if(dot(normal, in_unit_sphere) > 0) {
		return in_unit_sphere;
	} else {
		return -in_unit_sphere;
	}
}

bool float3_near_zero(float3 v) {
	const float s = 1e-8;
#ifdef OPENCL
	return( fabs(v.x) < s && fabs(v.y) < s && fabs(v.z) < s );
#else
	return( fabs(v.s[0]) < s && fabs(v.s[1]) < s && fabs(v.s[2]) < s );
#endif
}

float3 float3_reflect(float3 v, float3 n) {
	return v - 2 * dot(v, n) * n;
}

enum MaterialTypes {
	MATERIAL_LAMBERTIAN,
	MATERIAL_METAL
};
