#include "hit_record.h"
#include "sphere.h"
#include "camera.h"
#include "lambertian.h"
#include "dielectric.h"

#ifndef OPENCL
#include <assert.h>
#endif


#define MAX_SPHERES 5
#define MAX_MATERIAL_INSTANCES 16
struct Scene {
  int sphere_count;
  int lambertian_count;
  int dielectric_count;

  int image_width;
  int image_height;
  int samples_per_pixel;
  int max_depth;
  Camera cam;

  Sphere spheres[MAX_SPHERES];
  Lambertian lambertians[MAX_MATERIAL_INSTANCES];
  Dielectric dielectrics[MAX_MATERIAL_INSTANCES];

// C++ code
#ifndef OPENCL
  Scene() {
      image_width        = 1920;
      image_height       = 1080;
      samples_per_pixel  = 10;
      max_depth          = 50;
  }

#define PUSH_ELEMENT(ElementType, element, MAX_COUNT)   \
  Scene& push_##element(ElementType element) {          \
    assert(element##_count < MAX_COUNT);                \
    element##s[element##_count] = element;              \
    element##_count++;                                  \
    return *this;                                       \
  }                                                          

  PUSH_ELEMENT(Sphere, sphere, MAX_SPHERES);
  PUSH_ELEMENT(Lambertian, lambertian, MAX_MATERIAL_INSTANCES);
  PUSH_ELEMENT(Dielectric, dielectric, MAX_MATERIAL_INSTANCES);
#endif
};

