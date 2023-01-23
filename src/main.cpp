#include <CL/cl.h>
#include <CL/cl_platform.h>
#include <cstdlib>
#include <memory>
#include <stdio.h>
#include <stdlib.h>

#include "CLUtil.h"
#include "camera.h"
#include "crt.h"
#include "hit_record.h"
#include "hittable_list.h"
#include "interval.h"
#include "sphere.h"
#include "vec3.h"
#include "common.h"
#include "ppm.h"
#include "ray.h"

Vec3 ray_color(Ray r, const HittableList& world) {
	HitRecord rec;
	if(world.hit(r, Interval(0, infinity), rec)) {
		return (rec.normal+1) * 0.5;
	}

	Vec3 unit_direction = unit_vector(r.d);
	float a = 0.5*(unit_direction.y + 1.0);

	return interpolate(Vec3(0.5, 0.7, 1.0), Vec3(1, 1, 1), a);
}


// Basic drawing down
// Next: start raytracing
int main(void) {
  cl_int err;
	
  cl::Context context = setupCL();
  std::vector<cl::Device> devices = context.getInfo<CL_CONTEXT_DEVICES>();

  auto kernelPath = "src/kernels/test_kernel.clcpp";
  auto program = buildProgram(kernelPath, context, devices);

  if(!program.has_value()) {
    std::cerr << "Error loading kernel at : " << kernelPath << std::endl;
    return EXIT_FAILURE;
  }

  cl::Kernel kernel(program.value(), "render_kernel", &err);
  clErr(err);

  cl::ImageFormat format = cl::ImageFormat(CL_RGBA, CL_UNORM_INT8);
  const auto      image  = PPMImage::magenta(1920, 1080);

  cl::Image2D outputImage =
      cl::Image2D(context, CL_MEM_WRITE_ONLY, format, image.width, image.height, 0, nullptr, &err);
  clErr(err);


  /* cl::Buffer clObjects = cl::Buffer( */
  /*     context,  */
  /*     CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,  */
  /*     sceneObjects.size() * sizeof(Object),  */
  /*     sceneObjects.data() */
  /*   ); */

  kernel.setArg(0, image.width);
  kernel.setArg(1, image.height);
  kernel.setArg(2, sizeof(cl_mem), &outputImage);

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

int main2(void) {
	int width = 300, height = 169, samples_per_pixel = 100;
	u8* pixels = new u8[width * height * 3];

	Camera cam;

	HittableList world;
	world.add_sphere(std::make_shared<Sphere>(Point3(0,0,-1), 0.5));
	world.add_sphere(std::make_shared<Sphere>(Point3(0,-100.5,-1), 100));
	
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			Color pixel_color;
			for (int sample = 0; sample < samples_per_pixel; sample++) {
				float s = (float)(x + random_float()) / (width  - 1);
				float t = (float)(y + random_float()) / (height - 1);

				Ray r = cam.get_ray(s, t);
				pixel_color += ray_color(r, world);

			}
			/* write_pixel_rgb_vec3(x, y, pixels, width, height, pixel_color, samples_per_pixel); */
		}	
	}

	/* to_ppm(pixels, width, height, "./test.ppm"); */

	return 0;
}
