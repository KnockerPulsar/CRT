#pragma once

#include <vector>
#include <memory>
#include "hit_record.h"
#include "interval.h"
#include "sphere.h"

using std::vector, std::shared_ptr;

struct HittableList {
	vector<shared_ptr<Sphere>> spheres;
	// Add lists for other types here.

	void add_sphere(shared_ptr<Sphere> sphere) {
		spheres.push_back(sphere);
	}

	bool hit(const Ray& r, Interval ray_t, HitRecord& rec) const {
		return hit_spheres(r, ray_t, rec);
	}

	private:
	bool hit_spheres(const Ray& r, Interval ray_t, HitRecord& rec) const {
		HitRecord temp_rec;
		float closest_so_far = ray_t.max;
		bool hit_anything = false;

		for (const auto& sphere: spheres) {
			if(sphere->hit(r, interval(ray_t.min, closest_so_far), temp_rec)) {
				closest_so_far = temp_rec.t;
				rec = temp_rec;
				hit_anything = true;
			}
		}

		return hit_anything;
	}
};
