#ifndef OPENCL
#include "host/CLUtil.h"
#endif

typedef struct {
  float3 albedo;
  float fuzz;
} Metal;

Metal metal(float3 alb, float fuzz) {
  return (Metal) { alb, fuzz < 1? fuzz : 1 };
}

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
