#include "ray.h"
#include "cl_util.cl"

kernel void init_rays(uint width, uint height, global Ray* rays, global uint2* seeds) {
	const float aspect_ratio = (float)width / height;
	const float viewport_height = 2.0;
	const float viewport_width = aspect_ratio * viewport_height;
	const float focal_length = 1.0;
	
	float3 origin = (float3)(0, 0, 0);
	float3 horizontal = (float3)(viewport_width, 0, 0);
	float3 vertical = (float3)(0, viewport_height, 0);
	float3 lower_left_corner = origin - horizontal/2 - vertical/2 - (float3)(0, 0, focal_length);

	int2 pos = {get_global_id(0), get_global_id(1)};
	int thread_index = get_global_id(0) + width * get_global_id(1);
	
	uint2 seed = seeds[thread_index];

	float du = (random_float(&seed) - 0.5) * 2;
	float dv = (random_float(&seed) - 0.5) * 2;
	
	float u = (float)(pos.x+du) / (float)(width-1);
	float v = (float)(pos.y+dv) / (float)(height-1);
	
	rays[thread_index] = ray(origin, lower_left_corner + u * horizontal + (1-v)*vertical - origin, pos);

	seeds[thread_index] = seed;
}
