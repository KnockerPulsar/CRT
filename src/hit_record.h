#pragma once

#include "ray.h"
#include "cl_util.cl"

typedef struct { uint material_type, material_instance; } MaterialIdentifier;

MaterialIdentifier mat_id_lambertian(uint instance) { return (MaterialIdentifier){MATERIAL_LAMBERTIAN, instance}; }
MaterialIdentifier mat_id_metal(uint instance) { return (MaterialIdentifier){MATERIAL_METAL, instance}; }
MaterialIdentifier mat_id_dielectric(uint instance) { return (MaterialIdentifier){MATERIAL_DIELECTRIC, instance}; }

typedef struct {
	float3 p;
	float3 normal;
	float t;
	bool front_face;
	MaterialIdentifier mat_id;
} HitRecord;

void set_face_normal(HitRecord* hr, const Ray* r, const float3 outward_normal) {
	hr->front_face = dot(r->d, outward_normal) < 0;
	hr->normal = hr->front_face? outward_normal: -outward_normal;
}

HitRecord no_hit(int2 pixel) {
	return (HitRecord) { 0 };
}

bool hit_record_no_hit(HitRecord* hr) {
	return hr->t == 0;
}
