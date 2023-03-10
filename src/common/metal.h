#pragma once

#ifndef OPENCL
#include "host/CLUtil.h"
#include "host/CLBuffer.h"
#include "host/Utils.h"
#endif

#include "common/common_defs.h"
#include "common/material_id.h"

SHARED_STRUCT_START(Metal) {
  float3 albedo;
  float fuzz;
  MaterialId id;

#ifndef OPENCL
  public:
  class CLBuffer; friend CLBuffer;
  inline static std::vector<Metal> instances;

  static MaterialId fromAlbedoFuzz(float3 albedo, float fuzz) {
    auto metIter = std::find(instances.begin(), instances.end(), (Metal){albedo, fuzz});

    if(metIter != instances.end()) {
      return metIter->id;
    } else {
      return createAndPush(albedo, fuzz).id;
    }

  }

  bool operator==(const Metal& other) const {
    return float3_equals(this->albedo, other.albedo) && this->fuzz == other.fuzz;
  }

  private:
  static Metal createAndPush(float3 albedo, float fuzz) {
    Metal metal{albedo, fuzz};

    metal.id.material_type = MATERIAL_METAL;
    metal.id.material_instance = instances.size();

    instances.push_back(metal);

    return metal;
  }
#endif
} SHARED_STRUCT_END(Metal);


#ifdef OPENCL

#include "device/ray.h"
#include "device/hit_record.h"
#include "device/cl_util.cl"

bool metal_scatter(Metal* metal, const Ray* r_in, const HitRecord* rec, float3* attenuation, Ray* scattered, uint2* seed) {
  float3 reflected = float3_reflect(normalize(r_in->d), rec->normal);

  *scattered = ray(rec->p, reflected + metal->fuzz * random_in_unit_sphere(seed));
  *attenuation = metal->albedo;

  return(dot(scattered->d, rec->normal) > 0);
}
#endif
