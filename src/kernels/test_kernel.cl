#include "device/hit_record.h"
#include "device/ray.h"
#include "device/cl_util.cl"

#include "common/interval.h"
#include "common/sphere.h"
#include "common/lambertian.h"
#include "common/metal.h"
#include "common/dielectric.h"
#include "common/camera.h"
#include "common/bvh_node.h"
#include "common/texture.h"

#define MAX_DEPTH 64

float3 ray_color(
	Ray r,
	global Sphere* spheres,
	int sphere_count,

	global BVHNode* bvh_nodes,
	int bvh_size,

	int max_depth,
	uint2* seed,
	global Lambertian* lambertians,
	global Metal* metals,
	global Dielectric* dielectrics,

	global Texture* textures
) {
	max_depth = min(max_depth, MAX_DEPTH);
	float3 attenuation = (float3)(1, 1, 1);

	for(int i = 0; i < max_depth - 1; i++) {

		HitRecord rec;
		Interval ray_t = interval(0.001f, infinity);
#if 0
		bool hit = closest_hit(spheres, sphere_count, r, &ray_t, &rec);
#else
		bool hit = bvh_intersect(bvh_nodes, spheres, &r, &ray_t, &rec);
#endif

		if(hit) {
			Ray scattered;
			int mat_instance = rec.mat_id.material_instance;
			int mat_type = rec.mat_id.material_type;

			bool scatter = false;
			float3 color = (float3)(0, 0, 0);

			switch(mat_type) {
				case MATERIAL_LAMBERTIAN: {
					uint texture_index = rec.mat_id.texture_index;
					scatter = lambertian_scatter(texture_index, textures, &r, &rec, &color, &scattered, seed);
				} break;
				case MATERIAL_METAL: {
					scatter = metal_scatter(&metals[mat_instance], &r, &rec, &color, &scattered, seed);
				} break;
				case MATERIAL_DIELECTRIC: {
					scatter = dielectric_scatter(dielectrics[mat_instance], &r, &rec, &color, &scattered, seed);
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

	global BVHNode* bvh_nodes,
	uint bvh_size,

	global uint2* seeds,
	int max_depth,

	global Lambertian* lambertians,
	global Metal* metals,
	global Dielectric* dielectrics,

	global Texture* textures,

	Camera camera
) {
	const uint width = get_image_width(input);
	const uint height = get_image_height(input);
	
	float2 pos = {get_global_id(0), get_global_id(1)};

	/* printf("%f %f\n", pos.x, pos.y); */
	int thread_index = get_global_id(0) + width * get_global_id(1);
	
	uint2 seed = seeds[thread_index];

	float du = (random_float(&seed) - 0.5) * 2;
	float dv = (random_float(&seed) - 0.5) * 2;
	
	float u = (float)(pos.x+du) / (float)(width-1);
	float v = (float)(pos.y+dv) / (float)(height-1);
	
	Ray r = camera_get_ray(&camera, u, v, &seed);

	float4 prev_color = read_imagef(input, (int2)(pos.x, pos.y));
	float3 pixel_color = ray_color(r, spheres, sphere_count, bvh_nodes, bvh_size, max_depth, &seed, lambertians, metals, dielectrics, textures);

	float4 final_color = prev_color + (float4)(pixel_color, 1.0);
	
	write_imagef(input, (int2)(pos.x,pos.y), final_color);

	seeds[thread_index] = seed;
} 
