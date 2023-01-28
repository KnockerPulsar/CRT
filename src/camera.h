#pragma once

#include "crt.h"
#include <algorithm>

struct Camera {
	float3 origin, lower_left_corner;
	float3 horizontal, vertical;

	Camera() {
		float aspect_ratio = 16.0 / 9.0;
	
		float viewport_height = 2.0f;
		float viewport_width = aspect_ratio * viewport_height; 
		float focal_length = 1;

		origin = (float3){{0, 0, 0}};
		horizontal = (float3){{viewport_width, 0, 0}};
		vertical = (float3){{0, viewport_height, 0}};

		// auto lower_left_corner = origin - horizontal/2 - vertical/2 - vec3(0, 0, focal_length);
		lower_left_corner.v4 = origin.v4 - horizontal.v4/2 - vertical.v4/2 - (float3){{0, 0, focal_length}}.v4;
	}
	
	Ray get_ray(float s, float t) const {
		float3 direction;
		direction.v4 = lower_left_corner.v4 + s * horizontal.v4 + (1-t)*vertical.v4 - origin.v4;
		return ray(origin, direction);
	}
};
