#include "host/CLBuffer.h"
#include "host/CLUtil.h"

#include "host/builtin_scenes.h"
#include <CL/cl.h>
#include <optional>
#include <sched.h>

using std::vector, std::string;
using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::milliseconds;


void parseInt(int& var, const std::string& name, const char* intStr) {
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

void parseArguments(const char **argv, int argc, int &samplesPerPixel,
                    int &maxDepth, int &imageWidth, int &imageHeight,
                    std::string &outputFileName) {

  for(int i = 1; i < argc; i++) {
    if(STR_EQ(argv[i], "--samples")) {
      parseInt(samplesPerPixel, "samples", argv[i+1]);
      i += 1;
    } else if(STR_EQ(argv[i], "--max-depth")) {
      parseInt(maxDepth, "max-depth", argv[i+1]);
      i += 1;
    } else if(STR_EQ(argv[i], "--image-width")) {
      parseInt(imageWidth, "image-width", argv[i+1]);
      i += 1;
    } else if(STR_EQ(argv[i], "--image-height")) {
      parseInt(imageHeight, "image-height", argv[i+1]);
      i += 1;
    } else if(STR_EQ(argv[i], "-o") || STR_EQ(argv[i], "--output")) {
      outputFileName = std::string(argv[i+1]);
      i += 1;
    } else if (STR_EQ(argv[i], "--scene")) {
      // --scene is checked before this function in order to figure out which
      // scene to initially load. If the scene is checked here, the loading of
      // the scene after this function will override the argument values passed
      // to the program.
      i += 1; // Skip --scene and its argument.
    } else {
      int ret = EXIT_SUCCESS;

      // Arguments other than "--help" should indicate an error.
      if(!STR_EQ(argv[i], "--help")) {
        std::cerr << fmt("Unrecognized option: '%s'.\n", argv[i]);
        ret = EXIT_FAILURE;
      }

      std::cerr << "Usage:\n"
                << fmt("\t%s [--samples number] [--max-depth number] [--image-width number] [--image-height number] [{--output , -o} filename (default: output.ppm)] [--scene number]\n", argv[0]);

      std::cerr << "\nAvaialble scenes:\n"
                << "\t 0: Random spheres\n"
                << "\t 1: Two checkered spheres\n";

      std::exit(ret);
    }
  }
}

CLBuffer<uint2> generateSeeds(cl_context& context, cl_command_queue& queue, int imageWidth, int imageHeight) {
  CLBuffer<uint2> seeds(context, queue, CL_MEM_READ_WRITE, imageWidth * imageHeight);

  std::uniform_int_distribution<uint> distribution(0, UINT32_MAX);
  std::mt19937 generator{42};

  for (int i = 0; i < imageWidth * imageHeight; i++) {
    uint a = distribution(generator);
    uint b = distribution(generator);
    seeds.push_back(u2(a, b));
  }

  return seeds;
}

std::optional<int> parseSceneArguement(const char** argv, int argc) {
  for(int i = 0; i < argc; i++) {
    if(STR_EQ(argv[i], "--scene")) {
      int s;
      parseInt(s, "scene", argv[i+1]);
      return s;
    }
  }

  return std::nullopt;
}

int main(int argc, const char** argv) {
  srand(42);

  Camera cam;
  cam.focus_dist = 10.f;
  int imageWidth = 1920;
  int imageHeight = 1080;
  int samplesPerPixel = 100;
  int maxDepth = 10;
  int scene = 0;
  std::string outputFileName = "output.ppm";

  if(auto s = parseSceneArguement(argv, argc); s.has_value()) {
    scene = s.value();
  }

  switch(scene) {
    case 1: 
      two_checkered_spheres(cam, imageWidth, imageHeight, samplesPerPixel, maxDepth); break;
    case 0:
    default:
      random_spheres(cam, imageWidth, imageHeight, samplesPerPixel, maxDepth);        break;
  }

  parseArguments(argv, argc, samplesPerPixel, maxDepth, imageWidth, imageHeight, outputFileName);

  auto [context, queue, device] = setupCL();
  cl_kernel kernel = kernelFromFile("src/kernels/test_kernel.cl", context, device, {"./src"});
  
  auto seeds = generateSeeds(context, queue, imageWidth, imageHeight);
  seeds.uploadToDevice(context);

  // SAH doesn't really help with spheres.
  BVH bvh = BVH(Sphere::instances);

  auto image  = PPMImage::black(queue, context, imageWidth, imageHeight);
  image.write_to_device();

  CLBuffer<Sphere> spheres = CLBuffer<Sphere>::fromVector(context, queue, Sphere::instances); 
  CLBuffer<BVHNode> bvh_nodes = CLBuffer<BVHNode>::fromPtr(context, queue, bvh.getPool(), bvh.getNodesUsed());
  CLBuffer<Lambertian> lambertians = CLBuffer<Lambertian>::fromVector(context, queue, Lambertian::instances);
  CLBuffer<Metal> metals = CLBuffer<Metal>::fromVector(context, queue, Metal::instances);
  CLBuffer<Dielectric> dielectrics = CLBuffer<Dielectric>::fromVector(context, queue, Dielectric::instances);
  CLBuffer<Texture> textures = CLBuffer<Texture>::fromVector(context, queue, Texture::instances);

  cam.initialize((float)(imageWidth) / imageHeight);

  spheres.uploadToDevice(context);
  bvh_nodes.uploadToDevice(context);
  lambertians.uploadToDevice(context);
  metals.uploadToDevice(context);
  dielectrics.uploadToDevice(context);
  textures.uploadToDevice(context);

  kernelParameters(kernel, 0, image.clImage, spheres, spheres.count(), bvh_nodes, bvh_nodes.count(), seeds, maxDepth, lambertians, metals, dielectrics, textures, cam);

  cl_event event;
  std::array<size_t, 2> image_size{(std::size_t)image.width, (std::size_t)image.height};
  std::array<size_t, 2> zero_offset{0, 0};
  std::array<size_t, 2> local_work_size{16, 16};

  std::cout << fmt("Output file name: %s\n", outputFileName.c_str())
            << fmt("Raytracing with resolution: %dx%d, samples: %d, max depth: %d\n", imageWidth, imageHeight, samplesPerPixel, maxDepth)
            << fmt("# Spheres: %d, Lambertians: %d, Metals: %d, Dielectrics: %d\n", spheres.count(), lambertians.count(), metals.count(), dielectrics.count())
            << fmt("# Textures: %d\n", textures.count())
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
