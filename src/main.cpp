#include <memory>
#include <stdio.h>
#include <stdlib.h>

#include "camera.h"
#include "crt.h"
#include "hit_record.h"
#include "hittable_list.h"
#include "interval.h"
#include "sphere.h"
#include "vec3.h"
#include "common.h"
#include "ppm.h"
#include "ray.h"

Vec3 ray_color(Ray r, const HittableList& world) {
	HitRecord rec;
	if(world.hit(r, Interval(0, infinity), rec)) {
		return (rec.normal+1) * 0.5;
	}

	Vec3 unit_direction = unit_vector(r.d);
	float a = 0.5*(unit_direction.y + 1.0);

	return interpolate(Vec3(0.5, 0.7, 1.0), Vec3(1, 1, 1), a);
}


int main(void) {
	int width = 300, height = 169, samples_per_pixel = 100;
	u8* pixels = new u8[width * height * 3];

	Camera cam;

	HittableList world;
	world.add_sphere(std::make_shared<Sphere>(Point3(0,0,-1), 0.5));
	world.add_sphere(std::make_shared<Sphere>(Point3(0,-100.5,-1), 100));
	
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			Color pixel_color;
			for (int sample = 0; sample < samples_per_pixel; sample++) {
				float s = (float)(x + random_float()) / (width  - 1);
				float t = (float)(y + random_float()) / (height - 1);

				Ray r = cam.get_ray(s, t);
				pixel_color += ray_color(r, world);

			}
			write_pixel_rgb_vec3(x, y, pixels, width, height, pixel_color, samples_per_pixel);
		}	
	}

	to_ppm(pixels, width, height, "./test.ppm");

	return 1;
}
