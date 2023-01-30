#pragma once

#include "ray.h"

typedef struct { uint material_id; uint instance_id; } MaterialIdentifier;

typedef struct {
	float3 normal;
	Ray incident_ray;
	MaterialIdentifier materialId;
	float t;
	bool front_face;
	// Can get p by ray_at(incident_ray, t);
} HitRecord;

void set_face_normal(HitRecord* hr, const Ray* r, const float3 outward_normal) {
	hr->front_face = dot(r->d, outward_normal) < 0;
	hr->normal = hr->front_face? outward_normal: -outward_normal;
}

HitRecord no_hit(int2 pixel) {
	return (HitRecord) {
		(float3){0, 0, 0},
		ray((float3){0,0,0}, (float3){0,0,0}, pixel),
		(MaterialIdentifier){0, 0},
		0,
		false,
	};
}

bool hit_record_no_hit(HitRecord* hr) {
	return hr->t == 0;
}
