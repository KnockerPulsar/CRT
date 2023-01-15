#include <stdio.h>
#include <stdlib.h>

#include "vec3.h"
#include "common.h"
#include "ppm.h"


struct Ray {
	Vec3 o, d;

	Ray(Vec3 o, Vec3 d): o(o), d(d) {}
	Vec3 at(float t) { return o + d * t; }
};

float hit_sphere(Vec3 center, float radius, Ray r) {
		/* vec3 oc = r.origin() - center; */
		/* auto a = r.direction().length_squared(); */
		/* auto half_b = dot(oc, r.direction()); */
		/* auto c = oc.length_squared() - radius*radius; */
		/* auto discriminant = half_b*half_b - a*c; */
		/*  */
		/* if (discriminant < 0) { */
		/* 	return -1.0; */
		/* } else { */
		/* 	return (-half_b - sqrt(discriminant) ) / a; */
		/* } */

		Vec3 oc = r.o - center;
		float a = r.d.length_squared();
		float half_b = dot(oc, r.d);
		float c = oc.length_squared() - radius * radius;
		float discriminant = half_b * half_b - a*c;

		if(discriminant < 0) { 
			return -1; 
		} else {
			return (-half_b - sqrt(discriminant)) / a;
		}
}

Vec3 ray_color(Ray r) {
	float t = hit_sphere(Vec3(0, 0, -1), 0.5, r);
	if(t > 0) {
		Vec3 n = unit_vector(r.at(t) - Vec3(0,0, -1));
		return (n+1)* 0.5;
	}

	Vec3 unit_direction = unit_vector(r.d);
	float a = 0.5*(unit_direction.y + 1.0);

	return interpolate(Vec3(0.5, 0.7, 1.0), Vec3(1, 1, 1), a);
}


int main(void) {
	int width = 300, height = 300;
	float aspect_ratio = (float) width / height;
	u8* pixels = new u8[width * height * 3];

	float viewport_height = 2.0f;
	float viewport_width = aspect_ratio * viewport_height; 
	float focal_length = 1;

	Vec3 origin = Vec3(0, 0, 0);
	Vec3 horizontal = Vec3(viewport_width, 0, 0);
	Vec3 vertical = Vec3(0, viewport_height, 0);

	// auto lower_left_corner = origin - horizontal/2 - vertical/2 - vec3(0, 0, focal_length);
	Vec3 lower_left_corner = origin - horizontal/2 - vertical/2 - Vec3(0, 0, focal_length);
	
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			float s = (float)(x) / width;
			float t = (float)(y) / height;

			// ray dir = lower_left_corner + s*horizontal + (1-t)*vertical - origin
			Ray r = Ray(origin, lower_left_corner + s * horizontal + (1-t)*vertical - origin);
			Vec3 color = ray_color(r);
			

			write_pixel_rgb_vec3(x, y, pixels, width, height, color);
		}	
	}

	to_ppm(pixels, width, height, "./test.ppm");

	return 1;
}
