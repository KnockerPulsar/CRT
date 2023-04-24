#pragma once

#ifndef OPENCL
#include "host/CLUtil.h"
#include "host/CLBuffer.h"
#include "host/Utils.h"
#include <vector>
#include <algorithm>
#endif

#include "common/common_defs.h"
#include "common/material_id.h"

SHARED_STRUCT_START(Metal) {
  float3 albedo;
  float fuzz;
  MaterialId id;

#ifndef OPENCL
  MATERIAL_DEF(
      Metal, 
      MATERIAL_METAL,
      bool operator==(const Metal& other) const {
      return float3Equals(this->albedo, other.albedo) && this->fuzz == other.fuzz;
      }
  )
#endif
} SHARED_STRUCT_END(Metal);


#ifdef OPENCL

#include "device/ray.h"
#include "device/hit_record.h"
#include "device/cl_util.cl"

bool metal_scatter(global Metal* metal, const Ray* r_in, const HitRecord* rec, float3* attenuation, Ray* scattered, uint2* private  seed) {
  float3 reflected = float3_reflect(normalize(r_in->d), rec->normal);

  *scattered = ray(rec->p, reflected + metal->fuzz * random_in_unit_sphere(seed));
  *attenuation = metal->albedo;

  return(dot(scattered->d, rec->normal) > 0);
}
#endif
