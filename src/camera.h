#pragma once

#include "crt.h"
#include "vec3.h"
#include <algorithm>

struct Camera {
	Point3 origin, lower_left_corner;
	Vec3 horizontal, vertical;

	Camera() {
		float aspect_ratio = 16.0 / 9.0;
	
		float viewport_height = 2.0f;
		float viewport_width = aspect_ratio * viewport_height; 
		float focal_length = 1;

		origin = Vec3(0, 0, 0);
		horizontal = Vec3(viewport_width, 0, 0);
		vertical = Vec3(0, viewport_height, 0);

		// auto lower_left_corner = origin - horizontal/2 - vertical/2 - vec3(0, 0, focal_length);
		lower_left_corner = origin - horizontal/2 - vertical/2 - Vec3(0, 0, focal_length);

	}
	
	Ray get_ray(float s, float t) const {
		return Ray(origin, lower_left_corner + s * horizontal + (1-t)*vertical - origin);
	}
};
