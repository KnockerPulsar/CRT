#include "cl_util.cl"
#include "ray.h"
#include "hit_record.h"

kernel void scatter_diffuse(
	read_write image2d_t input,
	global HitRecord* hits,
	global Ray* scattered_rays,
	global uint* scattered_count,
	global uint2* seeds
) {
	float4 new_color = (float4)(1);
	int thread_id = get_global_id(0);
	HitRecord hr = hits[thread_id];
	uint2 seed = seeds[thread_id];

	float3 hit_point = ray_at(&hr.incident_ray, hr.t);
	float3 target = hit_point + hr.normal + random_unit_vector(&seed);
	uint scattered_index = atomic_inc(scattered_count);
	scattered_rays[scattered_index] = ray(hit_point, target - hit_point, hr.incident_ray.pixel);

	float4 old_color = read_imagef(input, hr.incident_ray.pixel);

	new_color *= 0.5f * old_color;

	write_imagef(input, hr.incident_ray.pixel, new_color);

	seeds[thread_id] = seed;
}
