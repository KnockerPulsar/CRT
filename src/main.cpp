#include "CLUtil.h"

#include <CL/cl.h>
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

#include "sphere.h"
#include "ppm.h"
#include "cl_buffer.h"

using std::vector, std::string;

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
  int numSamples = 100;
	
  auto [context, queue, devices] = setupCL();

  cl::Kernel pixelwise_divide = kernelFromFile("src/kernels/pixelwise_divide.cl", context, devices);
  
  cl::Kernel kernel = kernelFromFile("src/kernels/test_kernel.cl", context, devices, {"./src"});
  
  cl::ImageFormat format = cl::ImageFormat(CL_RGBA, CL_FLOAT);
  auto image  = PPMImage::magenta(imageWidth, imageHeight);
  cl::Image2D outputImage = cl::Image2D(context, CL_MEM_READ_WRITE, format, image.width, image.height, 0, nullptr, &err);
  clErr(err);
  

  CLBuffer<Sphere> spheres(context, queue, CL_MEM_READ_WRITE); 
  spheres
    .push_back({sphere({{0, 0, -1}}, 0.5f), sphere({{0, -100.5, -1}}, 100)})
    .uploadToDevice();
  
  CLBuffer<uint2> seeds(context, queue, CL_MEM_READ_WRITE, imageWidth * imageHeight);
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


  clErr(kernel.setArg(0, outputImage)); // Input image
  clErr(kernel.setArg(1, spheres.devBuffer()));
  clErr(kernel.setArg(2, spheres.count()));
  clErr(kernel.setArg(3, seeds.devBuffer()));
  
  cl::NDRange offset(0, 0, 0);
  cl::NDRange size((std::size_t)image.width, (std::size_t)image.height, 1);
  
  cl::Event event;

  std::cout << std::setfill('0') << std::setw(5) << std::fixed << std::setprecision(2);
  for (int i = 0 ; i < numSamples; i++) {
    std::cout << "\rSample progress: " << ((float)i/(numSamples-1)) * 100 << "%" << std::flush;
    clErr(queue.enqueueNDRangeKernel(kernel, offset, size, cl::NullRange, NULL, &event));
    event.wait();
  }
  std::cout << "\rDone.                            \n";

  const std::array<unsigned long, 3> origin = {0, 0, 0};
  const std::array<unsigned long, 3> region = {(size_t)image.width, (size_t)image.height, 1};
  
  
  pixelwise_divide.setArg(0, outputImage);
  pixelwise_divide.setArg(1, (float)numSamples);
  
  clErr(queue.enqueueNDRangeKernel(pixelwise_divide, offset, size, cl::NullRange, NULL));
  
  float* new_data = new float[imageWidth * imageHeight * 4];
  queue.enqueueReadImage(outputImage, CL_TRUE, origin, region, 0, 0, new_data);
  image.from_rgb_f32(new_data);

  image.write("output.ppm", numSamples);
  

	return EXIT_SUCCESS;
}
