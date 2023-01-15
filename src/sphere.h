#pragma once

#include "hit_record.h"
#include "interval.h"
#include "ray.h"
#include "vec3.h"

struct Sphere {
	Point3 center;
	float radius;

	Sphere(Point3 c, float r): center(c), radius(r) {}

	bool hit(const Ray& r, Interval ray_t, HitRecord& rec) {
		Vec3 oc = r.o - center;
		float a = r.d.length_squared();
		float half_b = dot(oc, r.d);
		float c = oc.length_squared() - radius * radius;

		float discriminant = half_b * half_b - a*c;
		if(discriminant < 0) return false;

		float sqrtd = sqrt(discriminant);
		float root = (-half_b - sqrtd) / a;

		if(!ray_t.contains(root)) {
			root = (-half_b + sqrtd) / a;
			if(!ray_t.contains(root)) {
				return false;
			}
		}

		rec.t = root;
		rec.p = r.at(root);
		Vec3 outward_normal = (rec.p - center) / radius;
		rec.set_face_normal(r, outward_normal);

		return true;
	}
};
