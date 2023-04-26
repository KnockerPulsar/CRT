#pragma once

#include "host/CLErrors.h"
#include "host/CLUtil.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <ostream>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>

#include <memory>
#include <random>
#include <string>
#include <iomanip>
#include <vector>
#include <array>
#include <utility>
#include <chrono>

#include "host/PPM.h"
#include "host/Utils.h"
#include "host/CLBuffer.h"
#include "host/CLMath.h"
#include "host/Random.h"
#include "host/BVH.h"
#include "host/CLKernel.h"

#include "common/sphere.h"
#include "common/camera.h"
#include "common/lambertian.h"
#include "common/metal.h"
#include "common/dielectric.h"
#include "common/material_id.h"
#include "common/texture.h"

void random_spheres(
    Camera& camera,
    int& imageWidth, int& imageHeight,
    int& samplesPerPixel, int& maxDepth
) {
  camera.lookfrom = f3(13, 2, 3);
  camera.lookat   = f3(0, 0, 0);
  camera.vup      = f3(0, 1, 0);
  camera.vfov     = 20;
  camera.aperature = 0.1;
  camera.focus_dist = 10;

  auto checker1 = Texture::CheckerTexture(0.32/2, f3(.2, .2, .2), f3(.8, .8, .8));
  auto checker2 = Texture::CheckerTexture(0.32/2, f3(.8, .2, .2), f3(.2, .2, .8));
  MaterialId groundTextureIndex = Lambertian::construct(Texture::CheckerTexture(.32, checker1, checker2));
  Sphere::addToScene(f3(0, -1000, 0), 1000, groundTextureIndex);

#if 1
  auto bounds = 11;
  for(int a = -bounds; a < bounds; a++) {
    for(int b = -bounds; b < bounds; b++) {
        float choose_mat = randomFloat();

        float3 center = f3(a + 0.9f * randomFloat(), randomFloatRanged(0.2f, 0.6f), b + 0.9f * randomFloat());

        if(length(center - f3(4, 0.2, 0)) > 0.9) {
          if(choose_mat < 0.8) {

            float3 albedo = randomFloat3() * randomFloat3();
            Sphere::addToScene(center, 0.2, Lambertian::construct(Texture::SolidColor(albedo)));

          } else if (choose_mat < 0.95) {
            float3 albedo = randomFloat3Ranged(0.5, 1);
            float fuzz = randomFloatRanged(0, 0.5);

            Sphere::addToScene(center, 0.2, Metal::push_back({albedo, fuzz}));
          } else {
            Sphere::addToScene(center, 0.2, Dielectric::push_back({randomFloatRanged(1.0, 10)}));
          }
      }
    }
  }

  Sphere::addToScene(f3(0, 1, 0), 1.0f, Dielectric::push_back({1.5}));
  Sphere::addToScene(f3(4, 1, 0), 1.0f, Metal::push_back({f3(0.7, 0.6, 0.5), 0.0}));
  Sphere::addToScene(f3(-4, 1, 0), 1, Lambertian::construct(Texture::SolidColor(f3(0.4, 0.2, 0.1))));
#endif

  // TODO: Add fallback/default elements in the buffers we upload.
  // 1. Can't leave it empty and try to upload (OpenCL gives a
  // `CL_INVALID_BUFFER_SIZE` when uploading 0 bytes).
  // 2. Can't not upload it (OpenCL segfaults when trying to upload the kernel
  // argument).
  /* Dielectric::push_back({1.5}); */
  /* Metal::push_back({f3(0.7, 0.6, 0.5), 0.0}); */
}

void two_checkered_spheres(
    Camera& camera,
    int& imageWidth, int& imageHeight,
    int& samplesPerPixel, int& maxDepth
) {
  imageWidth = 800;
  imageHeight = imageWidth * 9.0/16.f;
  samplesPerPixel = 100;

  camera.aperature = 0.1f;
  camera.vfov = 20.f;
  camera.lookfrom = f3(13, 2, 3);
  camera.lookat = f3(0);

  auto checker_id = Texture::CheckerTexture(0.8, f3(.2, .3, .1), f3(.9, .9, .9));

  Sphere::addToScene(f3(0, -10, 0), 10, Lambertian::construct(checker_id));
  Sphere::addToScene(f3(0,  10, 0), 10, Lambertian::construct(checker_id));

  Dielectric::push_back({1.5});
  Metal::push_back({f3(0.7, 0.6, 0.5), 0.0});
}
