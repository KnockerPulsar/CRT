#include "CLUtil.h"

#include <CL/cl.h>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <random>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iomanip>

#include "sphere.h"
#include "ppm.h"

using std::vector, std::string;

// Basic drawing down
// Next: start raytracing
int main(void) {
  cl_int err;

  int imageWidth = 1920;
  int imageHeight = 1080;
  int numSamples = 1000;
	
  cl::Context context = setupCL();
  std::vector<cl::Device> devices = context.getInfo<CL_CONTEXT_DEVICES>();
  
  auto test_program = buildProgram( "src/kernels/test_kernel.cl", context, devices, vector<string>{ "./src" });
  
  if(!test_program.has_value()) {
    return EXIT_FAILURE;
  }
  
  cl::Kernel kernel(test_program.value(), "render_kernel", &err);
  clErr(err);
  
  cl::ImageFormat format = cl::ImageFormat(CL_RGBA, CL_FLOAT);
  auto      image  = PPMImage::magenta(imageWidth, imageHeight);

  cl::Image2D outputImage = cl::Image2D(context, CL_MEM_READ_WRITE, format, image.width, image.height, 0, nullptr, &err);
  clErr(err);
  
  
  std::vector<Sphere> spheres;
  spheres.push_back(sphere({{0, 0, -1}}, 0.5f));
  spheres.push_back(sphere({{0, -100.5, -1}}, 100));
  
  cl::Buffer spheresBuffer = cl::Buffer(
      context, 
      CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, 
      spheres.size() * sizeof(Sphere), 
      spheres.data()
    );
  
  std::vector<uint2> seeds = std::vector<uint2>(image.width * image.height);
  std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
  std::mt19937 generator;
  
  for (int i = 0; i < image.width * image.height; i++) {
    float x = distribution(generator);
    float y = distribution(generator);
    uint a = (uint)(x * (float)UINT32_MAX);
    uint b = (uint)(y * (float)UINT32_MAX);
  
    seeds[i] = {{a, b}};
  }
  
  cl::Buffer seedsBuffer = cl::Buffer(
      context, 
      CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, 
      seeds.size() * sizeof(uint2), 
      seeds.data()
    );
  
  kernel.setArg(0, sizeof(cl_mem), &outputImage); // Input image
  kernel.setArg(1, spheresBuffer);
  kernel.setArg(2, spheres.size());
  kernel.setArg(3, seedsBuffer);
  
  cl::CommandQueue queue(context, devices[0], 0, &err);
  clErr(err);
  
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
  

  auto div_program = buildProgram("src/kernels/pixelwise_divide.cl", context, devices, vector<string>{ "./src" });
  if(!test_program.has_value()) {
    return EXIT_FAILURE;
  }
  
  cl::Kernel pixelwise_divide(div_program.value(), "pixelwise_divide", &err);
  clErr(err);
  
  pixelwise_divide.setArg(0, outputImage);
  pixelwise_divide.setArg(1, (float)numSamples);
  
  clErr(queue.enqueueNDRangeKernel(pixelwise_divide, offset, size, cl::NullRange, NULL));
  

  float* new_data = new float[imageWidth * imageHeight * 4];
  queue.enqueueReadImage(outputImage, CL_TRUE, origin, region, 0, 0, new_data);
  image.from_rgb_f32(new_data);

  image.write("output.ppm");

	return EXIT_SUCCESS;
}
