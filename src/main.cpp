#include "CLUtil.h"

#include <CL/cl.h>
#include <CL/cl_ext.h>
#include <CL/cl_platform.h>
#include <CL/opencl.hpp>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <random>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iomanip>
#include <vector>
#include <ranges>
#include <chrono>

#include "hit_record.h"
#include "sphere.h"
#include "ppm.h"
#include "cl_buffer.h"
#include "utils.h"
#include "lambertian.h"
#include "metal.h"

using std::vector, std::string;
using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::milliseconds;

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
  int numSamples = 10000;
  int maxDepth = 50;
	
  auto [context, queue, device] = setupCL();

  cl::Kernel pixelwise_divide = kernelFromFile("src/kernels/pixelwise_divide.cl", context, device);
  
  cl::Kernel kernel = kernelFromFile("src/kernels/test_kernel.cl", context, device, {"./src"});
  
  cl::ImageFormat format = cl::ImageFormat(CL_RGBA, CL_FLOAT);
  auto image  = PPMImage::magenta(imageWidth, imageHeight);
  cl::Image2D outputImage = cl::Image2D(context, CL_MEM_READ_WRITE, format, image.width, image.height, 0, nullptr, &err);
  clErr(err);
  

  CLBuffer<Sphere> spheres(context, queue, CL_MEM_READ_WRITE, 10 * 10 * 10); 
  CLBuffer<uint2> seeds(context, queue, CL_MEM_READ_WRITE, imageWidth * imageHeight);
  CLBuffer<Lambertian> lambertians(context, queue, CL_MEM_READ_ONLY);
  CLBuffer<Metal> metals(context, queue, CL_MEM_READ_ONLY);

  std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
  std::mt19937 generator;

  spheres
    .push_back({
        sphere({0, -100.5, -1}, 100, mat_id_lambertian(0)),
        sphere({0, 0, -1}, 0.5f, mat_id_lambertian(1)), 
        sphere({-1, 0, -1}, 0.5f, mat_id_metal(0)), 
        sphere({+1, 0, -1}, 0.5f, mat_id_metal(1)) 
    }).uploadToDevice();
  
  for (int i = 0; i < imageWidth * imageHeight; i++) {
    float x = distribution(generator);
    float y = distribution(generator);
    uint a = (uint)(x * (float)UINT32_MAX);
    uint b = (uint)(y * (float)UINT32_MAX);
  
    seeds.push_back((cl_uint2){{a, b}});
  }

  seeds.uploadToDevice();

  lambertians
    .push_back({lambertian({0.8, 0.8, 0.0}), lambertian({0.7, 0.3, 0.3})})
    .uploadToDevice();

  metals
    .push_back({metal({0.8, 0.8, 0.8}), metal({0.8, 0.6, 0.2})})
    .uploadToDevice();

  clErr(kernel.setArg(0, outputImage)); // Input image
  clErr(kernel.setArg(1, spheres.devBuffer()));
  clErr(kernel.setArg(2, spheres.count()));
  clErr(kernel.setArg(3, seeds.devBuffer()));
  clErr(kernel.setArg(4, maxDepth));
  clErr(kernel.setArg(5, lambertians.devBuffer()));
  clErr(kernel.setArg(6, metals.devBuffer()));
  
  cl::Event event;
  cl::NDRange image_size((std::size_t)image.width, (std::size_t)image.height, 1);
  cl::NDRange zero_offset(0, 0, 0);

  std::cout << fmt("Raytracing with resolution: %dx%d, samples: %d, max depth: %d\n", imageWidth, imageHeight, numSamples, maxDepth);
  std::cout << std::setfill('0') << std::setw(5) << std::fixed << std::setprecision(2);

  auto start = high_resolution_clock::now();
  for (int i = 0 ; i < numSamples; i++) {
    std::cout << "\rSample progress: " << ((float)i/(numSamples-1)) * 100 << "%" << std::flush;
    clErr(queue.enqueueNDRangeKernel(kernel, zero_offset, image_size, cl::NullRange, NULL, &event));
    event.wait();
  }
  auto end = high_resolution_clock::now();
  std::cout << "\rDone.                            \n";
  std::cout << fmt("Raytracing done in %d ms\n", duration_cast<milliseconds>(end-start));

  const std::array<unsigned long, 3> origin = {0, 0, 0};
  const std::array<unsigned long, 3> region = {(size_t)image.width, (size_t)image.height, 1};
  
  
  pixelwise_divide.setArg(0, outputImage);
  pixelwise_divide.setArg(1, (float)numSamples);
  
  clErr(queue.enqueueNDRangeKernel(pixelwise_divide, zero_offset, image_size));
  
  float* new_data = new float[imageWidth * imageHeight * 4];
  queue.enqueueReadImage(outputImage, CL_TRUE, origin, region, 0, 0, new_data);
  image.from_rgb_f32(new_data);

  image.write("output.ppm", numSamples);
  
	return EXIT_SUCCESS;
}
