#include "CLUtil.h"
#include "ray.h"
#include "hit_record.h"
#include "cl_util.cl"

typedef struct {
  float3 albedo;
} Metal;

Metal metal(float3 alb) {
  return (Metal) { alb };
}

bool metal_scatter(constant Metal* metal, const Ray* r_in, const HitRecord* rec, float3* attenuation, Ray* scattered, uint2* seed) {
  float3 reflected = float3_reflect(normalize(r_in->d), rec->normal);

  *scattered = ray(rec->p, reflected);
  *attenuation = metal->albedo;

  return(dot(scattered->d, rec->normal) > 0);
}
