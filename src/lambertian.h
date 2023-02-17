#pragma once

#include "CLUtil.h"
#include "hit_record.h"
#include "cl_util.cl"
#include "ray.h"

typedef struct {
  float3 albedo;
} Lambertian;

Lambertian lambertian(float3 alb) {
  return (Lambertian) { alb };
}

bool lambertian_scatter(constant Lambertian* lamb, const Ray* r_in, const HitRecord* rec, float3* attenuation, Ray* scattered, uint2* seed) {
  float3 scatter_direction = rec->normal + random_unit_vector(seed);

  if(float3_near_zero(scatter_direction)) {
    scatter_direction = rec->normal;
  }

  *scattered = ray(rec->p, scatter_direction);
  *attenuation = lamb->albedo;

  return true;
}
