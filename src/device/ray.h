#pragma once

#ifndef OPENCL
#include "host/CLUtil.h"
#include "host/CLMath.h"
#endif

typedef struct { float3 o, d; } Ray;

Ray ray(float3 o, float3 d) {
	return (Ray){ o , d };
}

float3 ray_at(const Ray* r, float t) { 
	return r->o + r->d * t;
}