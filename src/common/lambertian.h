#pragma once

#ifndef OPENCL
#include "host/Utils.h"
#include "host/CLUtil.h"
#include <vector>
#include <algorithm>
#endif

#include "common/material_id.h"
#include "common/common_defs.h"
#include "common/texture.h"

SHARED_STRUCT_START(Lambertian) {
  MaterialId id;

#ifndef OPENCL
  static MaterialId construct(uint textureId) {
    MaterialId id = { .texture_index = textureId };
    return Lambertian::push_back({id});
  }
  
  MATERIAL_DEF(
      Lambertian, 
      MATERIAL_LAMBERTIAN,
      bool operator==(const Lambertian& other) const {
        return id.texture_index == other.id.texture_index;
      }
  )
#endif
} SHARED_STRUCT_END(Lambertian);



#ifdef OPENCL
#include "device/hit_record.h"
#include "device/cl_util.cl"
#include "device/ray.h"

bool lambertian_scatter(uint texture_index, global const Texture* textures, const Ray* r_in, const HitRecord* rec, float3* attenuation, Ray* scattered, uint2* private seed) {
  float3 scatter_direction = rec->normal + random_unit_vector(seed);

  if(float3_near_zero(scatter_direction)) {
    scatter_direction = rec->normal;
  }

  *scattered = ray(rec->p, scatter_direction);
  *attenuation = texture_sample(texture_index, textures, rec->uv, rec->p);

  return true;
}
#endif
