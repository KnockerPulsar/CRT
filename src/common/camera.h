#pragma once

#ifndef OPENCL
#include "host/CLUtil.h"
#include "host/CLMath.h"
#include "common/common_defs.h"
#include <math.h>
#endif 

SHARED_STRUCT_START(Camera) {
	// Supplied by the user 
	float3 lookfrom, lookat, vup;
	float  vfov, aperature, focus_dist;

	// Cached. Initialized in `initialize`.
	float3 origin, lower_left_corner;
	float3 horizontal, vertical;
	float3 u, v, w;
	float lens_radius;


// C++ (host) code
#ifndef OPENCL 
	Camera() 
		: lookfrom(f3(0, 0, -1)), lookat(f3(0, 0, 0)), vup(f3(0, 1, 0)),
			vfov(40), aperature(0), focus_dist(10)
		{}

	void initialize(float aspect_ratio) {
			float theta = degrees_to_radians(vfov);
			float h = tan(theta/2);
			float viewport_height = 2.0f * h;
			float viewport_width = aspect_ratio * viewport_height; 

			w = normalize(lookfrom - lookat);
			u = normalize(cross(vup, w));
			v = cross(w, u);

			origin = lookfrom;
			horizontal = focus_dist * viewport_width * u;
			vertical = focus_dist * viewport_height * v;

			lower_left_corner = origin - horizontal/2 - vertical/2 - focus_dist * w;
			lens_radius = aperature / 2;
	}
#endif


} SHARED_STRUCT_END(Camera);


// OpenCL C (device) code
#ifdef OPENCL

#include "device/cl_util.cl"
#include "device/ray.h"

Ray camera_get_ray(const Camera* cam, float s, float t, uint2* seed) {
		float3 rd = cam->lens_radius * random_in_unit_disk(seed);
		float3 offset = cam->u * rd.x + cam->v * rd.y;

		float3 direction = 
			cam->lower_left_corner 
			+ s * cam->horizontal 
			+ (1-t) * cam->vertical 
			- cam->origin 
			- offset;

		return ray(cam->origin + offset, direction);
} 
#endif
