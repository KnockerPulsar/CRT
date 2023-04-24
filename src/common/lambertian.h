#pragma once

#ifndef OPENCL
#include "host/Utils.h"
#include "host/CLUtil.h"
#include <vector>
#include <algorithm>
#endif

#include "common/material_id.h"
#include "common/common_defs.h"

SHARED_STRUCT_START(Lambertian) {
  float3 albedo;
  MaterialId id;

#ifndef OPENCL
  MATERIAL_DEF(
      Lambertian, 
      MATERIAL_LAMBERTIAN,
      bool operator==(const Lambertian& other) const {
      return float3Equals(this->albedo, other.albedo);
      }
  )
#endif
} SHARED_STRUCT_END(Lambertian);


#ifdef OPENCL
#include "device/hit_record.h"
#include "device/cl_util.cl"
#include "device/ray.h"

bool lambertian_scatter(global Lambertian* lamb, const Ray* r_in, const HitRecord* rec, float3* attenuation, Ray* scattered, uint2* private seed) {
  float3 scatter_direction = rec->normal + random_unit_vector(seed);

  if(float3_near_zero(scatter_direction)) {
    scatter_direction = rec->normal;
  }

  *scattered = ray(rec->p, scatter_direction);
  *attenuation = lamb->albedo;

  return true;
}
#endif
