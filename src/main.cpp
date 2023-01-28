#include <CL/cl.h>
#include <CL/cl_platform.h>
#include <cstdlib>
#include <memory>
#include <stdio.h>
#include <stdlib.h>

#include "CLUtil.h"
#include "sphere.h"
#include "ppm.h"

// Basic drawing down
// Next: start raytracing
int main(void) {
  cl_int err;
	
  cl::Context context = setupCL();
  std::vector<cl::Device> devices = context.getInfo<CL_CONTEXT_DEVICES>();

  auto kernelPath = "src/kernels/test_kernel.clcpp";
  auto includes = std::vector<std::string>{ "./src" };
  auto program = buildProgram(kernelPath, context, devices, includes);

  if(!program.has_value()) {
    std::cerr << "Error loading kernel at : " << kernelPath << std::endl;
    return EXIT_FAILURE;
  }

  cl::Kernel kernel(program.value(), "render_kernel", &err);
  clErr(err);

  cl::ImageFormat format = cl::ImageFormat(CL_RGBA, CL_UNORM_INT8);
  const auto      image  = PPMImage::magenta(1920, 1080);

  cl::Image2D outputImage = cl::Image2D(context, CL_MEM_WRITE_ONLY, format, image.width, image.height, 0, nullptr, &err);
  clErr(err);


  /* std::vector<Sphere> spheres; */
  /* spheres.push_back(sphere({{0, 0, 0}}, 0.5f)); */
  /*  */
  /* cl::Buffer clObjects = cl::Buffer( */
  /*     context,  */
  /*     CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,  */
  /*     spheres.size() * sizeof(Sphere),  */
  /*     spheres.data() */
  /*   ); */

  kernel.setArg(0, image.width);
  kernel.setArg(1, image.height);
  kernel.setArg(2, sizeof(cl_mem), &outputImage);
  /* kernel.setArg(3, clObjects); */
  /* kernel.setArg(4, spheres.size()); */

  /* kernel.setArg(3, clObjects); */
  /* kernel.setArg(4, sceneObjects.size()); */

  cl::CommandQueue queue(context, devices[0], 0, &err);
  clErr(err);

  cl::NDRange offset(0, 0, 0);
  cl::NDRange size((std::size_t)image.width, (std::size_t)image.height, 1);

  cl::Event event;
  clErr(queue.enqueueNDRangeKernel(kernel, offset, size, cl::NullRange, NULL, &event));
  event.wait();

  const std::array<unsigned long, 3> origin = {0, 0, 0};
  const std::array<unsigned long, 3> region = {(size_t)image.width, (size_t)image.height, 1};

  queue.enqueueReadImage(outputImage, CL_TRUE, origin, region, 0, 0, image.data);

  image.write("output.ppm");

	return EXIT_SUCCESS;
}
