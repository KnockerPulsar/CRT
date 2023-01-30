#include "sphere.h"
#include "hit_record.h"
#include "ray.h"
#include "interval.h"
#include "cl_util.cl"

#define MAX_DEPTH 64
// Seems like we don't need gamma correction?

bool closest_hit(
	Ray r,
	constant Sphere* spheres,
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

void intersect(
	Ray r, 
	constant Sphere* spheres,
	int sphere_count,
	global HitRecord* hits,
	global uint* hit_count
	) {

	HitRecord rec;
	rec.incident_ray = r;
	bool hit_anything = closest_hit(r, spheres, sphere_count, interval(0.001f, infinity), &rec);

	if(hit_anything) {
		int hit_index = atomic_inc(hit_count);
		hits[hit_index] = rec;
	}
}

kernel void intersect_rays(
	global Ray* rays,
	constant Sphere* spheres, 
	int sphere_count,
	global HitRecord* hits,
	global uint* hit_count
) {
	
	uint thread_index = get_global_id(0);
	Ray r = rays[thread_index];

	intersect(r, spheres, sphere_count, hits, hit_count);
} 
