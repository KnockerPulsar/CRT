#include "host/CLUtil.h"

#include <CL/cl.h>
#include <CL/cl_ext.h>
#include <CL/cl_platform.h>
#include <CL/opencl.hpp>

#include <cstdint>
#include <cstdlib>
#include <stdio.h>
#include <stdlib.h>

#include <memory>
#include <random>
#include <string>
#include <iomanip>
#include <vector>
#include <utility>
#include <chrono>

#include "host/PPM.h"
#include "host/Utils.h"
#include "host/CLBuffer.h"
#include "host/CLMath.h"
#include "host/Random.h"

#include "common/sphere.h"
#include "common/camera.h"
#include "common/lambertian.h"
#include "common/metal.h"
#include "common/dielectric.h"
#include "common/material_id.h"

using std::vector, std::string;
using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::milliseconds;

void random_spheres(
    CLBuffer<Sphere>& spheres,
    CLBuffer<Lambertian>& lambertians,
    CLBuffer<Metal>& metals,
    CLBuffer<Dielectric>& dielectrics,
    Camera& camera,
    int& imageWidth, int& imageHeight,
    int& samplesPerPixel, int& maxDepth
) {
  imageWidth = 1920;
  imageHeight = 1080;
  samplesPerPixel = 100;
  maxDepth = 50;

  camera.lookfrom = f3(13, 2, 3);
  camera.lookat   = f3(0,  0, 0);
  camera.vup      = f3(0,  1, 0);
  camera.vfov     = 20;
  camera.aperature= 0.1;
  camera.focus_dist = 10;

  lambertians.push_back(lambertian(f3(0.5, 0.5, 0.5)));
  spheres.push_back(sphere(f3(0, -1000, 0), 1000, mat_id_lambertian(0)));


  auto c = 11;
  for(int a = -c; a < c; a++) {
    for(int b = -c; b < c; b++) {
      float choose_mat = random_float();

      float3 center = f3(a + 0.9f * random_float(), random_float_ranged(0.2, 2), b + 0.9f * random_float());

      if(length(center - f3(4, 0.2, 0)) > 0.9) {
        MaterialId sphere_material;

        if(choose_mat < 0.8) {
          float3 albedo = random_float3() * random_float3();

          uint lambertian_count = lambertians.count();
          sphere_material = mat_id_lambertian(lambertian_count);

          lambertians.push_back({albedo});
          spheres.push_back(sphere(center, 0.2, sphere_material));
        } else if (choose_mat < 0.95) {
          float3 albedo = random_float3_ranged(0.5, 1);
          float fuzz = random_float_ranged(0, 0.5);

          uint metal_count = metals.count();
          sphere_material = mat_id_metal(metal_count);

          metals.push_back({albedo, fuzz});
          spheres.push_back(sphere(center, 0.2, sphere_material));
        } else {
          uint dielectric_count = dielectrics.count();
          sphere_material = mat_id_dielectric(dielectric_count);

          dielectrics.push_back(dielectric(1.5));
          spheres.push_back(sphere(center, 0.2, sphere_material));
        }
      }
    }
  }

  MaterialId material1 = mat_id_dielectric(dielectrics.count());
  dielectrics.push_back(dielectric(1.5));
  spheres.push_back(sphere(f3(0, 1, 0), 1.0f, material1));

  MaterialId material2 = mat_id_lambertian(lambertians.count());
  lambertians.push_back(lambertian(f3(0.4, 0.2, 0.1)));
  spheres.push_back(sphere(f3(-4, 1, 0), 1.0f, material2));

  MaterialId material3 = mat_id_metal(metals.count());
  metals.push_back({f3(0.7, 0.6, 0.5), 0.0});
  spheres.push_back(sphere(f3(4, 1, 0), 1.0f, material3));
}

/*
 * for each sample:
 *    for each bounce:
 *      list geometry_hits; int counter;
 *      test_geometry(input_rays, spheres, max_bounces, geometry_hits, counter);
 *
 *      for hit in geometry_hits:
 *        list output_rays;
 *        scatter(materials, image, output_rays)
 */

int main(void) {
  cl_int err;

  int imageWidth = 1920;
  int imageHeight = 1080;
  int samplesPerPixel = 10000;
  int maxDepth = 500;

  Camera cam;
	
  auto [context, queue, device] = setupCL();

  cl::Kernel pixelwise_divide = kernelFromFile("src/kernels/pixelwise_divide.cl", context, device);
  
  cl::Kernel kernel = kernelFromFile("src/kernels/test_kernel.cl", context, device, {"./src"});
  
  srand(42);

  int preAllocObjs = 10;
  CLBuffer<Sphere> spheres(context, queue, CL_MEM_READ_WRITE, preAllocObjs); 
  CLBuffer<uint2> seeds(context, queue, CL_MEM_READ_WRITE, imageWidth * imageHeight);

  CLBuffer<Lambertian> lambertians(context, queue, CL_MEM_READ_ONLY, preAllocObjs);
  CLBuffer<Metal> metals(context, queue, CL_MEM_READ_ONLY, preAllocObjs);
  CLBuffer<Dielectric> dielectrics(context, queue, CL_MEM_READ_ONLY, preAllocObjs);

  std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
  std::mt19937 generator;

  for (int i = 0; i < imageWidth * imageHeight; i++) {
    float x = distribution(generator);
    float y = distribution(generator);
    uint a = (uint)(x * (float)UINT32_MAX);
    uint b = (uint)(y * (float)UINT32_MAX);
  
    seeds.push_back((cl_uint2){{a, b}});
  }

  seeds.uploadToDevice();

  random_spheres(
      spheres,
      lambertians,
      metals,
      dielectrics,
      cam,
      imageWidth, imageHeight,
      samplesPerPixel, maxDepth
  );

  cl::ImageFormat format = cl::ImageFormat(CL_RGBA, CL_FLOAT);
  auto image  = PPMImage::magenta(imageWidth, imageHeight);
#if 1
  cl::Image2D outputImage = cl::Image2D(context, CL_MEM_READ_WRITE, format, image.width, image.height, 0, nullptr, &err);
  clErr(err);

  cam.initialize((float)(imageWidth) / imageHeight);
  spheres.uploadToDevice();
  lambertians.uploadToDevice();
  metals.uploadToDevice();
  dielectrics.uploadToDevice();

  clErr(kernel.setArg(0, outputImage)); // Input image
  clErr(kernel.setArg(1, spheres.devBuffer()));
  clErr(kernel.setArg(2, spheres.count()));
  clErr(kernel.setArg(3, seeds.devBuffer()));
  clErr(kernel.setArg(4, maxDepth));
  clErr(kernel.setArg(5, lambertians.devBuffer()));
  clErr(kernel.setArg(6, metals.devBuffer()));
  clErr(kernel.setArg(7, dielectrics.devBuffer()));
  clErr(kernel.setArg(8, cam));
  
  cl::Event event;
  cl::NDRange image_size((std::size_t)image.width, (std::size_t)image.height, 1);
  cl::NDRange zero_offset(0, 0, 0);

  std::cout << fmt("Raytracing with resolution: %dx%d, samples: %d, max depth: %d\n", imageWidth, imageHeight, samplesPerPixel, maxDepth);
  std::cout << std::setfill('0') << std::setw(5) << std::fixed << std::setprecision(2);

  auto start = high_resolution_clock::now();
  for (int i = 0 ; i < samplesPerPixel; i++) {
    std::cout << "\rSample progress: " << ((float)i/(samplesPerPixel-1)) * 100 << "%" << std::flush;
    clErr(queue.enqueueNDRangeKernel(kernel, zero_offset, image_size, cl::NullRange, NULL, &event));
    event.wait();
  }
  auto end = high_resolution_clock::now();
  std::cout << "\rDone.                            \n";
  std::cout << fmt("Raytracing done in %d ms\n", duration_cast<milliseconds>(end-start));

  const std::array<unsigned long, 3> origin = {0, 0, 0};
  const std::array<unsigned long, 3> region = {(size_t)image.width, (size_t)image.height, 1};
  
  
  pixelwise_divide.setArg(0, outputImage);
  pixelwise_divide.setArg(1, (float)samplesPerPixel);
  
  clErr(queue.enqueueNDRangeKernel(pixelwise_divide, zero_offset, image_size));
  
  float* new_data = new float[imageWidth * imageHeight * 4];
  queue.enqueueReadImage(outputImage, CL_TRUE, origin, region, 0, 0, new_data);
  image.from_rgb_f32(new_data);
#endif

  image.write("output.ppm", 1);
  
	return EXIT_SUCCESS;
}
