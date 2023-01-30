#pragma once

#include "hit_record.h"
#include "interval.h"
#include "ray.h"
#include "CLUtil.h"
#include "hit_record.h"

typedef struct {
	float3 center;
	float radius;
	MaterialIdentifier mat_id;
} Sphere;

Sphere sphere(float3 c, float r) {
	return (Sphere) {c, r, {0, 0}};
}

bool sphere_hit(constant Sphere* s, const Ray* r, Interval ray_t, HitRecord* rec) {
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

	float3 p = ray_at(r, root);
	float3 outward_normal = (p - s->center) / s->radius;

	rec->t = root;
	set_face_normal(rec, r, outward_normal);
	rec->materialId = s->mat_id;
	rec->incident_ray = *r;

	return true;
}
