#pragma once

#ifndef OPENCL
#include "host/CLUtil.h"
#endif


#include "common/common_defs.h"
#include "common/material_id.h"

SHARED_STRUCT_START(Dielectric) {
  float ir;
  MaterialId id;
  
#ifndef OPENCL
  public:
  class CLBuffer; friend CLBuffer;
  inline static std::vector<Dielectric> instances;

  static MaterialId fromIR(float ir) {
    auto diIter = std::find(instances.begin(), instances.end(), (Dielectric){ir});

    if(diIter != instances.end()) {
      return diIter->id;
    } else {
      return createAndPush(ir).id;
    }

  }

  bool operator==(const Dielectric& other) const {
    return this->ir == other.ir;
  }

  private:
  static Dielectric createAndPush(float ir) {
    Dielectric dielectric{ir};

    dielectric.id.material_type = MATERIAL_DIELECTRIC;
    dielectric.id.material_instance = instances.size();

    instances.push_back(dielectric);

    return dielectric;
  }
#endif
} SHARED_STRUCT_END(Dielectric);

#ifdef OPENCL

#include "device/cl_util.cl"
#include "device/ray.h"
#include "device/hit_record.h"

static float dielectric_reflectence(float cosine, float ref_idx) {
  float r0 = (1 - ref_idx) / (1 + ref_idx);
  r0 = r0 * r0;
  return r0 + (1-r0) * pow((1-cosine), 5);
}

bool dielectric_scatter(Dielectric dielec, const Ray* r_in, const HitRecord* rec, float3* attenuation, Ray* scattered, uint2* private  seed) {
  *attenuation = (float3){1, 1, 1};

  float refraction_ratio = rec->front_face? (1.0f/dielec.ir) : dielec.ir;

  float3 unit_direction = normalize(r_in->d);
  float cos_theta = fmin(dot(-unit_direction, rec->normal), 1.0f);
  float sin_theta = sqrt(1 - cos_theta * cos_theta);

  bool cannot_refract = refraction_ratio * sin_theta > 1;
  bool random_reflection = dielectric_reflectence(cos_theta, refraction_ratio) > random_float(seed);

  float3 direction = (cannot_refract || random_reflection)? 
      float3_reflect(unit_direction, rec->normal) 
    : float3_refract(unit_direction, rec->normal, refraction_ratio);


  *scattered = ray(rec->p, direction);

  return true;
}
#endif
