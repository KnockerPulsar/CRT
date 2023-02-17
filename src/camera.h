#pragma once

#include "CLUtil.h"
#include "cl_util.cl"
#include "ray.h"

#ifndef OPENCL 
#include <math.h>
#endif 

SHARED_STRUCT_START(Camera) {
	float3 origin, lower_left_corner;
	float3 horizontal, vertical;
	float3 lookfrom, lookat, vup;
	float  vfov;

// C++ (host) code
#ifndef OPENCL

	Camera() : vfov(40), lookfrom({0, 0, -1}), lookat({0, 0, 0}), vup({0, 1, 0}) {}

	void initialize(float aspect_ratio) {
			float theta = degrees_to_radians(vfov);
			float h = tan(theta/2);
			float viewport_height = 2.0f * h;
			float viewport_width = aspect_ratio * viewport_height; 

			float3 w = normalize(lookfrom - lookat);
			float3 u = normalize(cross(vup, w));
			float3 v = cross(w, u);

			origin = lookfrom;
			horizontal = viewport_width * u;
			vertical = viewport_height * v;

			// auto lower_left_corner = origin - horizontal/2 - vertical/2 - vec3(0, 0, focal_length);
			lower_left_corner = origin - horizontal/2 - vertical/2 - w;
	}
#endif

} SHARED_STRUCT_END(Camera);


// OpenCL C (device) code
Ray camera_get_ray(const Camera* cam, float s, float t) {
		float3 direction;
		direction = cam->lower_left_corner + s * cam->horizontal + (1-t) * cam->vertical - cam->origin;
		return ray(cam->origin, direction);
} 
