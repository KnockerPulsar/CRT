#include "CLUtil.h"

#include <CL/cl.h>
#include <CL/cl_platform.h>
#include <CL/opencl.hpp>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <random>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iomanip>
#include <sys/types.h>
#include <vector>

#include "hit_record.h"
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
  int numSamples = 1000;
  int maxDepth = 50;
	
  auto [context, queue, devices] = setupCL();

  // Init host side buffers
  cl::ImageFormat format = cl::ImageFormat(CL_RGBA, CL_FLOAT);
  auto accumImage  = PPMImage::magenta(imageWidth, imageHeight);
  cl::Image2D outputAccumImage = cl::Image2D(context, CL_MEM_READ_WRITE, format, accumImage.width, accumImage.height, 0, nullptr, &err);
  clErr(err);

  auto sampleImage  = PPMImage::magenta(imageWidth, imageHeight);
  cl::Image2D outputSampleImage = cl::Image2D(context, CL_MEM_READ_WRITE, format, sampleImage.width, sampleImage.height, 0, nullptr, &err);
  clErr(err);

  CLBuffer<Sphere> spheres(context, queue, CL_MEM_READ_WRITE); 
  CLBuffer<uint2> seeds(context, queue, CL_MEM_READ_WRITE, imageWidth * imageHeight);
  CLBuffer<Ray> rays(context, queue, CL_MEM_READ_WRITE, imageWidth * imageHeight);
  CLBuffer<HitRecord> hits(context, queue, CL_MEM_READ_WRITE, imageWidth * imageHeight);
  CLBuffer<uint> hit_count(context, queue, CL_MEM_READ_WRITE, 1);
  CLBuffer<uint> scattered_count(context, queue, CL_MEM_READ_WRITE, 1);

  std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
  std::mt19937 generator;
  
  spheres
    .push_back({sphere({{0, 0, -1}}, 0.5f), sphere({{0, -100.5, -1}}, 100)})
    .uploadToDevice();
  
  for (int i = 0; i < imageWidth * imageHeight; i++) {
    float x = distribution(generator);
    float y = distribution(generator);
    uint a = (uint)(x * (float)UINT32_MAX);
    uint b = (uint)(y * (float)UINT32_MAX);
  
    seeds.push_back((cl_uint2){{a, b}});

    int2 pixel = {i % imageWidth, i/imageWidth};
    hits.push_back(no_hit(pixel));
    rays.push_back(ray({0}, {0}, pixel));
  }
  seeds.uploadToDevice();

  rays.uploadToDevice();
  hits.uploadToDevice();
  hit_count.push_back(0).uploadToDevice();
  scattered_count.push_back(imageWidth * imageHeight).uploadToDevice();

  cl::Kernel draw_background = kernelFromFile("src/kernels/draw_background.cl", context, devices);
  cl::Kernel init_rays = kernelFromFile("src/kernels/init_rays.cl", context, devices, {"./src"});
  cl::Kernel intersect_rays = kernelFromFile("src/kernels/intersect_rays.cl", context, devices, {"./src"});
  cl::Kernel scatter_diffuse = kernelFromFile("src/kernels/scatter_diffuse.cl", context, devices, {"./src"});
  cl::Kernel pixelwise_add = kernelFromFile("src/kernels/pixelwise_add.cl", context, devices);
  cl::Kernel pixelwise_divide = kernelFromFile("src/kernels/pixelwise_divide.cl", context, devices);
  
  clErr(draw_background.setArg(0, outputSampleImage));
  clErr(draw_background.setArg(1, numSamples));

  clErr(init_rays.setArg(0, imageWidth));
  clErr(init_rays.setArg(1, imageHeight));
  clErr(init_rays.setArg(2, rays.devBuffer()));;
  clErr(init_rays.setArg(3, seeds.devBuffer()));

  clErr(intersect_rays.setArg(0, rays.devBuffer()));
  clErr(intersect_rays.setArg(1, spheres.devBuffer()));
  clErr(intersect_rays.setArg(2, spheres.count()));
  clErr(intersect_rays.setArg(3, hits.devBuffer()));
  clErr(intersect_rays.setArg(4, hit_count.devBuffer()));

  clErr(scatter_diffuse.setArg(0, outputSampleImage));
  clErr(scatter_diffuse.setArg(1, hits.devBuffer()));
  clErr(scatter_diffuse.setArg(2, rays.devBuffer()));
  clErr(scatter_diffuse.setArg(3, scattered_count.devBuffer()));
  clErr(scatter_diffuse.setArg(4, seeds.devBuffer()));

  clErr(pixelwise_add.setArg(0, outputAccumImage));
  clErr(pixelwise_add.setArg(1, outputSampleImage));

  clErr(pixelwise_divide.setArg(0, outputAccumImage));
  clErr(pixelwise_divide.setArg(1, (float)numSamples));
  
  cl::Event event;
  cl::NDRange zero_offset(0, 0, 0);
  cl::NDRange image_size((std::size_t)accumImage.width, (std::size_t)accumImage.height, 1);


#if 1
  std::cout << std::setfill('0') << std::setw(5) << std::fixed << std::setprecision(2);
  for (int i = 0 ; i < numSamples; i++) {

    clErr(queue.enqueueNDRangeKernel(draw_background, zero_offset, image_size));
    clErr(queue.enqueueNDRangeKernel(init_rays, zero_offset, image_size));

    scattered_count[0] = imageWidth * imageHeight; scattered_count.uploadToDevice();

    for(int j = 0; j < maxDepth; j++) {
      std::cout << "\rSample progress: " << ((float)i/(numSamples-1)) * 100 << "%" << std::flush;

      // Initialized to width * height
      cl::NDRange size(scattered_count[0], 1, 1);
      hit_count[0] = 0;
      hit_count.uploadToDevice();
      clErr(queue.enqueueNDRangeKernel(intersect_rays, zero_offset, size, cl::NullRange, NULL, &event));
      hit_count.readFromDevice();

      if(hit_count[0] == 0) {
        break;
      }
      /* std::cout << "[" << i << "] " << "Hit count: " << hit_count[0] << std::endl; */


      cl::NDRange hit_size(hit_count[0], 1, 1);
      scattered_count[0] = 0;
      scattered_count.uploadToDevice();
      clErr(queue.enqueueNDRangeKernel(scatter_diffuse, zero_offset, hit_size, cl::NullRange, NULL, &event));
      scattered_count.readFromDevice();
      
      if(scattered_count[0] == 0) {
        break;
      }
      /* std::cout << "[" << i << "] " << "Scattered count: " << scattered_count[0] << std::endl; */
    }

    clErr(queue.enqueueNDRangeKernel(pixelwise_add, zero_offset, image_size));
  }
  std::cout << "\rDone.                            \n";
#endif

  const std::array<unsigned long, 3> origin = {0, 0, 0};
  const std::array<unsigned long, 3> region = {(size_t)accumImage.width, (size_t)accumImage.height, 1};
  
  clErr(queue.enqueueNDRangeKernel(pixelwise_divide, zero_offset, image_size, cl::NullRange, NULL));
  
  float* new_data = new float[imageWidth * imageHeight * 4];
  queue.enqueueReadImage(outputAccumImage, CL_TRUE, origin, region, 0, 0, new_data);
  accumImage.from_rgb_f32(new_data);

  accumImage.write("output.ppm", numSamples);
  

	return EXIT_SUCCESS;
}
