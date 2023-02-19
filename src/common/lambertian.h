#pragma once

#ifndef OPENCL
#include "host/CLUtil.h"
#endif

typedef struct {
  float3 albedo;
} Lambertian;

Lambertian lambertian(float3 alb) {
  return (Lambertian) { alb };
}

#ifdef OPENCL

#include "device/hit_record.h"
#include "device/cl_util.cl"
#include "device/ray.h"

bool lambertian_scatter(Lambertian* lamb, const Ray* r_in, const HitRecord* rec, float3* attenuation, Ray* scattered, uint2* seed) {
  float3 scatter_direction = rec->normal + random_unit_vector(seed);

  if(float3_near_zero(scatter_direction)) {
    scatter_direction = rec->normal;
  }

  *scattered = ray(rec->p, scatter_direction);
  *attenuation = lamb->albedo;

  return true;
}
#endif
