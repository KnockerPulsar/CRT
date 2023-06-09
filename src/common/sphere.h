#pragma once
#include "common/aabb.h"
#include "common/common_defs.h"
#include "common/material_id.h"
#include "common/interval.h"

#ifndef OPENCL
#include "host/CLUtil.h"
#include "host/Utils.h"
#include <algorithm>
#endif


SHARED_STRUCT_START(Sphere) {
	float3 center;
	float radius;
	AABB bbox;
	MaterialId mat_id;

#ifndef OPENCL
	public:
	class CLBuffer; friend CLBuffer;

	inline static std::vector<Sphere> instances;

	Sphere(float3 center, float radius, MaterialId id): center(center), radius(radius), mat_id(id) {
		float3 rvec = f3(radius, radius, radius);
		bbox = AABB(center - rvec, center + rvec);
	}

	static void addToScene(float3 center, float radius, MaterialId id) {
		auto sphIter = std::find(instances.begin(), instances.end(), Sphere(center, radius, id));

		if(sphIter == instances.end()) {
			createAndPush(center, radius, id);
		}
	}

	bool operator==(const Sphere& other) const {
		return float3Equals(this->center, other.center) 
			&& this->radius == other.radius 
			&& materialIdEquals(this->mat_id, other.mat_id);
	}

	private:
	static void createAndPush(float3 center, float radius, MaterialId id) {
		Sphere sphere{center, radius, id};
		instances.push_back(sphere);
	}
#endif
} SHARED_STRUCT_END(Sphere);


#ifdef OPENCL
#include "device/hit_record.h"
#include "device/ray.h"

float2 compute_uvs(float3 n) {
	// float theta = acos(-n.y);
	// v = theta / pi;
	
	// float phi = atan2(-n.z, n.x) + pi;
	// u = phi / (2*pi);
	
	// acospi or atan2pi compute their respective fucntions 
	// but divide over pi.
	// For v this translates to one function call
	// For u, this needs some more work
	// 		u = (atan2(-n.z, n.x) + pi) / (2 * pi)
	// 		since atan2pi = atant2 / pi, we just divide the whole expression by pi
	// 		u = (atan2pi(-n.z, n.x) + 1) / 2
	float v = acospi(-n.y);
	float u = (atan2pi(-n.z, n.x) + 1) * 0.5f;
	return (float2)(u, v);
}

bool sphere_hit(global Sphere* s, const Ray* r, const Interval* ray_t, HitRecord* rec) {
	float3 oc = r->o - s->center;

	float a = dot(r->d, r->d);
	float half_b = dot(oc, r->d);
	float c = dot(oc, oc) - s->radius * s->radius;

	float discriminant = half_b * half_b -  a*c;
	if(discriminant < 0) return false;

	float sqrtd = sqrt(discriminant);
	float root = (-half_b - sqrtd) / a;
	
	if(!interval_contains(ray_t, root)) {
		root = (-half_b + sqrtd) / a;
		if(!interval_contains(ray_t, root)) {
			return false;
		}
	}

	rec->t = root;
	rec->p = ray_at(r, root);
	
	float3 outward_normal = (rec->p - s->center) / s->radius;
	rec->uv = compute_uvs(outward_normal);
	set_face_normal(rec, r, outward_normal);

	rec->mat_id = s->mat_id;

	return true;
}
#endif
