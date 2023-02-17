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
	float  vfov;

// C++ (host) code
#ifndef OPENCL

	Camera() : vfov(40) {}

	void initialize(float aspect_ratio) {
			float theta = degrees_to_radians(vfov);
			float h = tan(theta/2);
			float viewport_height = 2.0f * h;
			float viewport_width = aspect_ratio * viewport_height; 
			float focal_length = 1;

			origin = (float3){0, 0, 0};
			horizontal = (float3){viewport_width, 0, 0};
			vertical = (float3){0, viewport_height, 0};

			// auto lower_left_corner = origin - horizontal/2 - vertical/2 - vec3(0, 0, focal_length);
			lower_left_corner = origin - horizontal/2 - vertical/2 - (float3){0, 0, focal_length};
	}
#endif

} SHARED_STRUCT_END(Camera);


// OpenCL C (device) code
Ray camera_get_ray(const Camera* cam, float s, float t) {
		float3 direction;
		direction = cam->lower_left_corner + s * cam->horizontal + (1-t) * cam->vertical - cam->origin;
		return ray(cam->origin, direction);
} 
