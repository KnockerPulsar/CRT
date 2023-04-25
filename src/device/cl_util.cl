#pragma once

#ifdef OPENCL
#include "cl_def.cl"

// https://stackoverflow.com/a/16077942
float random_float(uint2* private  seed) {

	const float invMaxInt = 1.0f/4294967296.0f;

	uint x = (*seed).x * 17 + (*seed).y * 13123;
	(*seed).x = (x<<13) ^ x;
	(*seed).y ^= (x<<7);

	uint tmp =  
		(x * (x * x * 15731 + 74323) + 871483) 
		+ (x * (x * x * 13734 + 37828) + 234234) 
		+ (x * (x * x * 11687 + 26461) + 137589);

	return convert_float(tmp) * invMaxInt;
}

float random_float_ranged(uint2* private  seed1, float min, float max) {
	return min + (max-min) * random_float(seed1);
}

float3 random_float3(uint2* private  seed) {
	return (float3){random_float(seed), random_float(seed), random_float(seed)};
}

float3 random_float3_ranged(uint2* private  seed, float min, float max) {
	return (float3){
		random_float_ranged(seed, min, max), 
		random_float_ranged(seed, min, max), 
		random_float_ranged(seed, min, max)
	};
}

float3 random_in_unit_sphere(uint2* private  seed) {
	while(true) {
		float3 p = random_float3_ranged(seed, -1, 1);
		if(length(p) * length(p) >= 1) continue;

		return p;
	}
}

float3 random_unit_vector(uint2* private  seed) {
	return normalize(random_in_unit_sphere(seed));
}

float3 random_in_hemisphere(float3 normal, uint2* private  seed) {
	float3 in_unit_sphere = random_in_unit_sphere(seed);

	if(dot(normal, in_unit_sphere) > 0) {
		return in_unit_sphere;
	} else {
		return -in_unit_sphere;
	}
}

float3 random_in_unit_disk(uint2* private seed) {
	while(true) {
		float3 p = (float3){random_float_ranged(seed, -1, 1), random_float_ranged(seed, -1, 1), 0.0f};
		if(length(p) * length(p) >= 1) continue;

		return p;
	}
}

bool float3_near_zero(float3 v) {
	const float s = 1e-8;
	return( fabs(v.x) < s && fabs(v.y) < s && fabs(v.z) < s );
}

float3 float3_reflect(float3 v, float3 n) {
	return v - 2 * dot(v, n) * n;
}

float3 float3_refract(float3 uv, float3 n, float etai_over_etat) {
    float cos_theta = fmin(dot(-uv, n), 1.0f);
    float3 r_out_perp =  etai_over_etat * (uv + cos_theta*n);

	float perp_length_squared = length(r_out_perp) * length(r_out_perp);
    float3 r_out_parallel = -sqrt(fabs(1.0f - perp_length_squared)) * n;
    /* printf("%v3f, %v3f\n", r_out_perp, r_out_parallel); */
    return r_out_perp + r_out_parallel;
}

#define SWAP(a, b, type) \
	{ type c = a; a = b; b = c; }

#include "common/sphere.h"
#include "common/interval.h"
#include "device/hit_record.h"
bool closest_hit(
	global Sphere* spheres,
	int sphere_count,
	Ray r,
	Interval* ray_t,
	HitRecord* rec
) {
	HitRecord temp_rec;
	bool hit_anything = false;

	for(int i = 0; i < sphere_count; i++) {
		if(sphere_hit(&spheres[i], &r, ray_t, &temp_rec)) {
			hit_anything = true;
			ray_t->max = min(ray_t->max, temp_rec.t);
			*rec = temp_rec;
		}	
	}

	return hit_anything;
}


#define POP_STACK(var_to_pop_into) \
			if(stack_ptr == 0) break;  \
			var_to_pop_into = stack[--stack_ptr]

#define PUSH_STACK(value) \
	stack[stack_ptr++] = value;

#endif

