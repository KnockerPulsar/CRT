#pragma once

#include "device/ray.h"
#include "device/cl_util.cl"
#include "common/material_id.h"

typedef struct {
	float3 p;
	float3 normal;
	float t;
	bool front_face;
	MaterialId mat_id;
} HitRecord;

void set_face_normal(HitRecord* hr, const Ray* r, const float3 outward_normal) {
	hr->front_face = dot(r->d, outward_normal) < 0;
	hr->normal = hr->front_face? outward_normal: -outward_normal;
}
