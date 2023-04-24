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

  Sphere::addToScene(f3(0, -1000, 0), 1000, Lambertian::push_back({f3(0.5, 0.5, 0.5)}));
#if 1
  auto bounds = 10;
  for(int a = -bounds; a < bounds; a++) {
    for(int b = -bounds; b < bounds; b++) {
        float choose_mat = randomFloat();

        float3 center = f3(a + 0.9f * randomFloat(), randomFloatRanged(0.2f, 0.6f), b + 0.9f * randomFloat());

        if(length(center - f3(4, 0.2, 0)) > 0.9) {
          if(choose_mat < 0.8) {

            float3 albedo = randomFloat3() * randomFloat3();
            Sphere::addToScene(center, 0.2, Lambertian::push_back({albedo}));

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
#endif

  Sphere::addToScene(f3(0, 1, 0), 1.0f, Dielectric::push_back({1.5}));
  Sphere::addToScene(f3(4, 1, 0), 1.0f, Metal::push_back({f3(0.7, 0.6, 0.5), 0.0}));
  Sphere::addToScene(f3(-4, 1, 0), 1, Lambertian::push_back({f3(0.4, 0.2, 0.1)}));
}

void parseInt(int& var, const std::string& name, char* intStr) {
#define BAD_ARG() \
    std::cerr << fmt("Invalid value for the argument \"%s\" (%s). Aborting\n", name.c_str(), intStr); \
    std::exit(EXIT_FAILURE)

  try {
    var = std::stoi(intStr);
  } catch(std::invalid_argument const& e) {
    BAD_ARG();
  }

  if(var < 0) { BAD_ARG(); }
}

int main(int argc, char** argv) {
  srand(42);

  int imageWidth = 1920;
  int imageHeight = 1080;
  int samplesPerPixel = 1000;
  int maxDepth = 10;
  std::string outputFileName = "output.ppm";

#define STR_EQ(str1, str2) (std::strcmp((str1), (str2)) == 0)

  for(int i = 1; i < argc; i++) {
    if(STR_EQ(argv[i], "--samples")) {
      parseInt(samplesPerPixel, "samples", argv[i+1]);
      i += 1;
    } else if(STR_EQ(argv[i], "--max-depth")) {
      parseInt(maxDepth, "max-depth", argv[i+1]);
      i += 1;
    }
    else if(STR_EQ(argv[i], "--image-width")) {
      parseInt(imageWidth, "image-width", argv[i+1]);
      i += 1;
    } else if(STR_EQ(argv[i], "--image-height")) {
      parseInt(imageHeight, "image-height", argv[i+1]);
      i+= 1;
    } else if(STR_EQ(argv[i], "-o") || STR_EQ(argv[i], "--output")) {
      outputFileName = std::string(argv[i+1]);
      i+= 1;
    } else {
      int ret = EXIT_SUCCESS;

      // Arguments other than "--help" should indicate an error.
      if(!STR_EQ(argv[i], "--help")) {
        std::cerr << fmt("Unrecognized option: '%s'.\n", argv[i]);
        ret = EXIT_FAILURE;
      }

      std::cerr << "Usage:\n"
                << fmt("\t%s [--samples number] [--max-depth number] [--image-width number] [--image-height number] [{--output , -o} filename (default: output.ppm)]\n", argv[0]);

      std::exit(ret);
    }
  }

  auto [context, queue, device] = setupCL();

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

  Camera cam;
  random_spheres(cam, imageWidth, imageHeight, samplesPerPixel, maxDepth);

  // SAH doesn't really help with spheres.
  BVH bvh = BVH(Sphere::instances);

  auto image  = PPMImage::black(queue, context, imageWidth, imageHeight);
  image.write_to_device();

  CLBuffer<Sphere> spheres = CLBuffer<Sphere>::fromVector(context, queue, Sphere::instances); 
  CLBuffer<BVHNode> bvh_nodes = CLBuffer<BVHNode>::fromPtr(context, queue, bvh.getPool(), bvh.getNodesUsed());
  CLBuffer<Lambertian> lambertians = CLBuffer<Lambertian>::fromVector(context, queue, Lambertian::instances);
  CLBuffer<Metal> metals = CLBuffer<Metal>::fromVector(context, queue, Metal::instances);
  CLBuffer<Dielectric> dielectrics = CLBuffer<Dielectric>::fromVector(context, queue, Dielectric::instances);

  cam.initialize((float)(imageWidth) / imageHeight);

  spheres.uploadToDevice(context);
  bvh_nodes.uploadToDevice(context);
  lambertians.uploadToDevice(context);
  metals.uploadToDevice(context);
  dielectrics.uploadToDevice(context);

  kernelParameters(kernel, 0, image.clImage, spheres, spheres.count(), bvh_nodes, bvh_nodes.count(), seeds, maxDepth, lambertians, metals, dielectrics, cam);

  cl_event event;
  std::array<size_t, 2> image_size{(std::size_t)image.width, (std::size_t)image.height};
  std::array<size_t, 2> zero_offset{0, 0};
  std::array<size_t, 2> local_work_size{16, 16};

  std::cout << fmt("Output file name: %s\n", outputFileName.c_str())
            << fmt("Raytracing with resolution: %dx%d, samples: %d, max depth: %d\n", imageWidth, imageHeight, samplesPerPixel, maxDepth)
            << fmt("# Spheres: %d, Lambertians: %d, Metals: %d, Dielectrics: %d\n", spheres.count(), lambertians.count(), metals.count(), dielectrics.count())
            << std::setfill('0') << std::setw(5) << std::fixed << std::setprecision(2);

  auto start = high_resolution_clock::now();
  for (int i = 0 ; i < samplesPerPixel; i++) {
    auto current_time = duration_cast<milliseconds>(high_resolution_clock::now()-start);

    // Specifying a local workgroup size doesn't seem to improve performance at all..
    clErr(clEnqueueNDRangeKernel(queue, kernel, 2, zero_offset.data(), image_size.data(), local_work_size.data(), 0, NULL, &event));
    clErr(clWaitForEvents(1, &event));

    auto percentage = ((i + 1.f)/samplesPerPixel) * 100.f;
    std::cout << fmt("\r[%d ms] Sample progress: %.2f%%", current_time.count(), percentage) << std::flush;
  }
  auto end = high_resolution_clock::now();
  std::cout << "\rDone.                                        \n"
            << fmt("Raytracing done in %d ms\n", duration_cast<milliseconds>(end-start));
  
  image.read_from_device();
  image.write_to_file(outputFileName, samplesPerPixel);
  
	return EXIT_SUCCESS;
}
