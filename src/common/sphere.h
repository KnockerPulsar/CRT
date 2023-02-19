#pragma once
#include "common/common_defs.h"
#include "common/material_id.h"

#ifndef OPENCL
#include "host/CLUtil.h"
#endif


typedef struct {
	float3 center;
	float radius;
	MaterialId mat_id;
} Sphere;

Sphere sphere(float3 c, float r, MaterialId mat_id) {
	return (Sphere) {c, r, mat_id};
}


#ifdef OPENCL

#include "device/hit_record.h"
#include "device/interval.h"
#include "device/ray.h"

bool sphere_hit(Sphere* s, const Ray* r, Interval ray_t, HitRecord* rec) {
	float3 oc = r->o - s->center;

	float a = dot(r->d, r->d);
	float half_b = dot(oc, r->d);
	float c = dot(oc, oc) - s->radius * s->radius;

	float discriminant = half_b * half_b -  a*c;
	if(discriminant < 0) return false;

	float sqrtd = sqrt(discriminant);
	float root = (-half_b - sqrtd) / a;
	
	if(!interval_contains(&ray_t, root)) {
		root = (-half_b + sqrtd) / a;
		if(!interval_contains(&ray_t, root)) {
			return false;
		}
	}

	rec->t = root;
	rec->p = ray_at(r, root);
	
	float3 outward_normal = (rec->p - s->center) / s->radius;
	set_face_normal(rec, r, outward_normal);

	rec->mat_id = s->mat_id;

	return true;
}

#endif
