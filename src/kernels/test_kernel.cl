#include "sphere.h"
#include "hit_record.h"
#include "ray.h"
#include "interval.h"
#include "cl_util.cl"
#include "lambertian.h"
#include "metal.h"

#define MAX_DEPTH 64
// Seems like we don't need gamma correction?

bool closest_hit(
	Ray r,
	Sphere* spheres,
	int sphere_count,
	Interval ray_t,
	HitRecord* rec
) {
	HitRecord temp_rec;
	bool hit_anything = false;
	float closest_so_far = ray_t.max;

	for(int i = 0; i < sphere_count; i++) {
		if(sphere_hit(&spheres[i], &r, ray_t, &temp_rec)) {
			hit_anything = true;
			ray_t.max = temp_rec.t;
			*rec = temp_rec;
		}	
	}

	return hit_anything;
}

float3 ray_color(Ray r, Sphere* spheres, int sphere_count, int max_depth, uint2* seed, constant Lambertian* lambertians, constant Metal* metals) {

	max_depth = min(max_depth, MAX_DEPTH);

	float3 attenuation = (float3)(1, 1, 1);

	for(int i = 0; i < max_depth - 1; i++) {
		HitRecord rec;
		if(closest_hit(r, spheres, sphere_count, interval(0.001f, infinity), &rec)) {
			Ray scattered;
			int mat_instance = rec.mat_id.material_instance;
			int mat_type = rec.mat_id.material_type;
			bool scatter = false;
			float3 color = (float3)(0, 0, 0);

			switch(mat_type) {
				case MATERIAL_LAMBERTIAN: 
					{
						scatter = lambertian_scatter(&lambertians[mat_instance], &r, &rec, &color, &scattered, seed);
					} break;
				case MATERIAL_METAL:
					{
						scatter = metal_scatter(&metals[mat_instance], &r, &rec, &color, &scattered, seed);
					} break;
			}

			if(!scatter) {
				return (float3)(0, 0, 0);
			}

			r = scattered;
			attenuation *= color;
		} else {
			float3 unit_direction = normalize(r.d);
			float a = 0.5 * (unit_direction.y + 1.0);
	
			attenuation *= ((1.0f-a) * (float3)(1, 1, 1) + a * (float3)(0.5, 0.7, 1.0));
			break;
		} 	
	}
	
	return attenuation;
}

kernel void test_kernel(
	read_write image2d_t input,
	global Sphere* spheres, 
	int sphere_count,
	global uint2* seeds,
	int max_depth,
	constant Lambertian* lambertians,
	constant Metal* metals
) {
	const uint width = get_image_width(input);
	const uint height = get_image_height(input);

	const float aspect_ratio = (float)width / height;
	const float viewport_height = 2.0;
	const float viewport_width = aspect_ratio * viewport_height;
	const float focal_length = 1.0;
	
	float3 origin = (float3)(0, 0, 0);
	float3 horizontal = (float3)(viewport_width, 0, 0);
	float3 vertical = (float3)(0, viewport_height, 0);
	float3 lower_left_corner = origin - horizontal/2 - vertical/2 - (float3)(0, 0, focal_length);
	
	float2 pos = {get_global_id(0), get_global_id(1)};
	int thread_index = get_global_id(0) + width * get_global_id(1);
	
	private uint2 seed = seeds[thread_index];

	float du = (random_float(&seed) - 0.5) * 2;
	float dv = (random_float(&seed) - 0.5) * 2;
	
	float u = (float)(pos.x+du) / (float)(width-1);
	float v = (float)(pos.y+dv) / (float)(height-1);
	
	Ray r = ray(origin, lower_left_corner + u * horizontal + (1-v)*vertical - origin);

	float4 prev_color = read_imagef(input, (int2)(pos.x, pos.y));
	float3 pixel_color = ray_color(r, spheres, sphere_count, max_depth, &seed, lambertians, metals);

	float4 final_color = prev_color + (float4)(pixel_color, 1.0);
	
	write_imagef(input, (int2)(pos.x,pos.y), final_color);

	seeds[thread_index] = seed;
} 
