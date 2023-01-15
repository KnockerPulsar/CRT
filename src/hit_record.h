#pragma once

#include "ray.h"
#include "vec3.h"

struct HitRecord {
	Point3 p;
	Vec3 normal;
	float t;
	bool front_face;

	void set_face_normal(const Ray& r, const Vec3& outward_normal) {
		front_face = dot(r.d, outward_normal) < 0;
		normal = front_face? outward_normal: -outward_normal;
	}
};
