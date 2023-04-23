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
#include "common/bvh_node.h"

using std::vector, std::string;
using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::milliseconds;

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

  Sphere::addToScene(f3(0, -1000, 0), 1000, Lambertian::fromAlbedo(f3(0.5, 0.5, 0.5)));
#if 1
  auto bounds = 11;
  for(int a = -bounds; a < bounds; a++) {
    for(int b = -bounds; b < bounds; b++) {
        float choose_mat = random_float();

        float3 center = f3(a + 0.9f * random_float(), random_float_ranged(0.2f, 0.6f), b + 0.9f * random_float());

        if(length(center - f3(4, 0.2, 0)) > 0.9) {
          if(choose_mat < 0.8) {

            float3 albedo = random_float3() * random_float3();
            Sphere::addToScene(center, 0.2, Lambertian::fromAlbedo(albedo));

          } else if (choose_mat < 0.95) {
            float3 albedo = random_float3_ranged(0.5, 1);
            float fuzz = random_float_ranged(0, 0.5);

            Sphere::addToScene(center, 0.2, Metal::fromAlbedoFuzz(albedo, fuzz));
          } else {
            Sphere::addToScene(center, 0.2, Dielectric::fromIR(random_float_ranged(1.0, 10)));
          }
      }
    }
  }
#endif

  Sphere::addToScene(f3(0, 1, 0), 1.0f, Dielectric::fromIR(1.5));
  Sphere::addToScene(f3(4, 1, 0), 1.0f, Metal::fromAlbedoFuzz(f3(0.7, 0.6, 0.5), 0.0));
  Sphere::addToScene(f3(-4, 1, 0), 1, Lambertian::fromAlbedo(f3(0.4, 0.2, 0.1)));

  Dielectric::fromIR(1.5f);
  Metal::fromAlbedoFuzz(f3(1, 1, 1), 1);
  Lambertian::fromAlbedo(f3(.8, .8, .8));
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

void parseInt(int& var, const std::string& name, char** argv, int& i) {
#define BAD_ARG() \
    std::cerr << fmt("Invalid value for the argument \"%s\" (%s). Aborting\n", name.c_str(), argv[i+1]); \
    std::exit(EXIT_FAILURE);

  try {
    var = std::stoi(argv[i+1]);
  } catch(std::invalid_argument const& e) {
    BAD_ARG()
  }

  if(var < 0) { BAD_ARG() }

  i++;
}

int main(int argc, char** argv) {
  srand(42);

  int imageWidth = 1920;
  int imageHeight = 1080;
  int samplesPerPixel = 1000;
  int maxDepth = 10;

  for(int i = 1; i < argc; i++) {
    if(std::strcmp(argv[i], "--samples") == 0) {
      parseInt(samplesPerPixel, "samples", argv, i);
    } else if(std::strcmp(argv[i], "--max-depth") == 0) {
      parseInt(maxDepth, "max-depth", argv, i);
    }
    else if(std::strcmp(argv[i], "--image-width") == 0) {
      parseInt(imageWidth, "image-width", argv, i);
    } else if(std::strcmp(argv[i], "--image-height") == 0) {
      parseInt(imageHeight, "image-height", argv, i);
    } else {
      std::cerr << fmt("Unrecognized option: '%s'.", argv[i]);
      std::cerr << "Usage:\n";
      std::cerr << "\tCRT [--samples number] [--max-depth number] [--image-width number] [--image-height number]\n";
      std::exit(EXIT_FAILURE);
    }
  }

  Camera cam;
	
  auto [context, queue, device] = setupCL();

  cl_kernel pixelwise_divide = kernelFromFile("src/kernels/pixelwise_divide.cl", context, device);
  cl_kernel kernel = kernelFromFile("src/kernels/test_kernel.cl", context, device, {"./src"});
  
  CLBuffer<uint2> seeds(context, queue, CL_MEM_READ_WRITE, imageWidth * imageHeight);

  std::uniform_int_distribution<uint> distribution(0, UINT32_MAX);
  std::mt19937 generator{42};

  for (int i = 0; i < imageWidth * imageHeight; i++) {
    uint a = distribution(generator);
    uint b = distribution(generator);
    seeds.push_back((cl_uint2){{a, b}});
  }

  seeds.uploadToDevice(context);

  random_spheres(cam, imageWidth, imageHeight, samplesPerPixel, maxDepth);

  int root_node_index = 0;
  int nodes_used = 0;
  BVHNode* pool = nullptr;
  BVHNode bvh = BVHNode(Sphere::instances, root_node_index, nodes_used, &pool);

  auto image  = PPMImage::black(queue, context, imageWidth, imageHeight);
  image.write_to_device();

  CLBuffer<Sphere> spheres = CLBuffer<Sphere>::fromVector(context, queue, Sphere::instances); 
  CLBuffer<BVHNode> bvh_nodes = CLBuffer<BVHNode>::fromPtr(context, queue, pool, nodes_used);
  CLBuffer<Lambertian> lambertians = CLBuffer<Lambertian>::fromVector(context, queue, Lambertian::instances);
  CLBuffer<Metal> metals = CLBuffer<Metal>::fromVector(context, queue, Metal::instances);
  CLBuffer<Dielectric> dielectrics = CLBuffer<Dielectric>::fromVector(context, queue, Dielectric::instances);

  cam.initialize((float)(imageWidth) / imageHeight);
  spheres.uploadToDevice(context);
  bvh_nodes.uploadToDevice(context);
  lambertians.uploadToDevice(context);
  metals.uploadToDevice(context);
  dielectrics.uploadToDevice(context);


  uint spheres_count = spheres.count();
  kernel_parameters(kernel, 0, image.cl_image, spheres, spheres_count, bvh_nodes, nodes_used, seeds, maxDepth, lambertians, metals, dielectrics, cam);

  cl_event event;
  std::array<size_t, 3> image_size{(std::size_t)image.width, (std::size_t)image.height, 1};
  std::array<size_t, 3> zero_offset{0, 0, 0};

  std::cout << fmt("Raytracing with resolution: %dx%d, samples: %d, max depth: %d\n", imageWidth, imageHeight, samplesPerPixel, maxDepth);
  std::cout << fmt("# Spheres: %d, Lambertians: %d, Metals: %d, Dielectrics: %d\n", spheres.count(), lambertians.count(), metals.count(), dielectrics.count());
  std::cout << std::setfill('0') << std::setw(5) << std::fixed << std::setprecision(2);

  auto start = high_resolution_clock::now();
  for (int i = 0 ; i < samplesPerPixel; i++) {

    auto current_time = duration_cast<milliseconds>(high_resolution_clock::now()-start);

    // Specifying a local workgroup size doesn't seem to improve performance at all..
    /* cl_int clEnqueueNDRangeKernel (	*/
    /*     cl_command_queue command_queue,  */
    /*     cl_kernel kernel, */
    /*     cl_uint work_dim, */
    /*     const size_t *global_work_offset, */
    /*     const size_t *global_work_size, */
    /*     const size_t *local_work_size, */
    /*     cl_uint num_events_in_wait_list, */
    /*     const cl_event *event_wait_list, */
    /*     cl_event *event) */

    clErr(clEnqueueNDRangeKernel(
          queue,
          kernel,
          2,
          zero_offset.data(),
          image_size.data(),
          NULL,
          0,
          NULL,
          &event
    ));
    clWaitForEvents(1, &event);

    auto percentage = ((i + 1.f)/samplesPerPixel) * 100.f;
    std::cout << fmt("\r[%d ms] Sample progress: %.2f%%", current_time.count(), percentage) << std::flush;
  }
  auto end = high_resolution_clock::now();
  std::cout << "\rDone.                            \n";
  std::cout << fmt("Raytracing done in %d ms\n", duration_cast<milliseconds>(end-start));
  
  image.read_from_device();

  image.write_to_file("output.ppm", samplesPerPixel);
  free(pool);
  
	return EXIT_SUCCESS;
}
