#pragma once

#include "common/dielectric.h"
#include "common/material_id.h"
#include "common/metal.h"
#include "common/lambertian.h"
#include "common/sphere.h"
#include "common/camera.h"


#ifndef OPENCL
#include <assert.h>
#include "host/CLBuffer.h"
#include <CL/opencl.hpp>
#include <vector>
#endif

// TODO: Add a function uploadToDevice(ctx, q) which uploads all 4 buffers here

namespace {

struct Scene {
  int image_width;
  int image_height;
  int samples_per_pixel;
  int max_depth;

  Camera cam;
  CLBuffer<Sphere> spheres;
  CLBuffer<Lambertian> lambertians;
  CLBuffer<Metal> metals;
  CLBuffer<Dielectric> dielectrics;

// C++ code
#ifndef OPENCL
  Scene(cl::Context& ctx, cl::CommandQueue& q) 
    : spheres(ctx, q), lambertians(ctx, q), metals(ctx, q), dielectrics(ctx, q) 
  {
      image_width        = 1920;
      image_height       = 1080;
      samples_per_pixel  = 10;
      max_depth          = 50;
  }

#define PUSH_MATERIAL(Type, typeVec)                                \
  template<>                                                        \
  MaterialId& Scene::pushMaterial(Type&& e) {                       \
		auto iter = std::find(typeVec.begin(), typeVec.end(), e);       \
		if(iter == typeVec.end()) {                                     \
			typeVec.push_back(e);                                         \
      return e.id;                                                  \
		}                                                               \
    return (*iter).id;                                              \
  }                                                                 

  template<typename T>
  T& pushGeometry(T&& elem) { assert(false && "Shouldn't use this! Please specialize."); }

  template<typename T>
  MaterialId& pushMaterial(T&& elem) { assert(false && "Shouldn't use this! Please specialize."); }

#endif
};

#ifndef OPENCL 
template<>
Sphere& Scene::pushGeometry(Sphere&& e) {                                     
  auto iter = std::find(spheres.begin(), spheres.end(), e);       
  if(iter == spheres.end()) {                                     
    spheres.push_back(e);                                         
    return e;                                                     
  }                                                               
  return *iter;                                                   
}                                                                 

  PUSH_MATERIAL(Lambertian, lambertians)
  PUSH_MATERIAL(Dielectric, dielectrics)
  PUSH_MATERIAL(Metal, metals)
};
#endif

