#include "CLUtil.h"
#include "utils.h"

#include <CL/cl.h>
#include <CL/cl_ext.h>
#include <CL/cl_platform.h>
#include <CL/opencl.hpp>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iterator>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <tuple>
#include <vector>
#include <filesystem>
#include <assert.h>

using std::string, std::cout, std::vector, std::cerr, std::endl;

template<typename T, typename U>
T getClInfo(U& platformOrDevice, cl_platform_info infoToGet) {
  T info;
  platformOrDevice.getInfo(infoToGet, &info);

  return info;
}

auto computeRubbishMips(cl::Device& device) {
    uint cus  = getClInfo<uint>(device, CL_DEVICE_MAX_COMPUTE_UNITS);
    uint freq = getClInfo<uint>(device, CL_DEVICE_MAX_CLOCK_FREQUENCY);

    return cus * freq;
}

auto getDevices(cl::Platform& platform) {
  vector<cl::Device> platformDevices; 
  clErr(platform.getDevices(CL_DEVICE_TYPE_ALL, &platformDevices));

  return platformDevices;
}

auto printSize(const vector<uint>&& sizes) {
  std::stringstream ss;
  ss << "[ ";
  for (auto& item : sizes) {
    ss << item << ", ";
  }
  ss << " ]";
  return ss.str();
}


auto printDevice(cl::Device& device) {
    uint mips = computeRubbishMips(device);
    cout << "\tDevice name: " << getClInfo<string>(device, CL_DEVICE_NAME) << '\n';
    cout << "\tDevice CUs: " <<  getClInfo<uint>(device, CL_DEVICE_MAX_COMPUTE_UNITS) << '\n';
    cout << "\tDevice max frequency: " <<  getClInfo<uint>(device, CL_DEVICE_MAX_CLOCK_FREQUENCY) << '\n';
    cout << "\tDevice max workgroup size: " <<  printSize(getClInfo<vector<uint>>(device, CL_DEVICE_MAX_WORK_GROUP_SIZE)) << '\n';
    cout << "\tDevice max workitem size: " <<  printSize(getClInfo<vector<uint>>(device, CL_DEVICE_MAX_WORK_ITEM_SIZES)) << '\n';
    cout << "\tDevice max constant parameters: " <<  printSize(getClInfo<vector<uint>>(device, CL_DEVICE_MAX_CONSTANT_ARGS)) << '\n';
    cout << "\tDevice RubbishMIPs: " << mips << "\n\n";
}

// https://stackoverflow.com/a/10031155
auto replaceAll(const string& source, string toBeReplaced, string replacement) {
  int pos;
  string sourceCopy = source;
  while ((size_t)(pos = sourceCopy.find(toBeReplaced)) != std::string::npos)
    sourceCopy.replace(pos, toBeReplaced.length(), replacement);

  return sourceCopy;
}

auto setupCL() -> std::tuple<cl::Context, cl::CommandQueue, cl::Device> {
  // So for some reason,
  // Fetching all devices from all platforms
  // THEN creating the context works fine on my UHD 630 
  // but nukes itself on my GTX 1660Ti.
  // So to get all devices, I'd have to create multiple contextes...


  cl_int err;

  vector<cl::Platform> platforms;
  clErr(cl::Platform::get(&platforms));

  uint maxRubbishMips = 0;
  cl::Device maxRubbishMipsDevice;
  for(auto& platform : platforms) {
    cout << "\nPlatform name: " << getClInfo<string>(platform, CL_PLATFORM_NAME) << "\n";

    /* string ext = getClInfo<string>(platform, CL_PLATFORM_EXTENSIONS);  */
    /* cout << "\nPlatform extensions: " << ext << "\n"; */

    vector<cl::Device> devices = getDevices(platform);

    for (int i = 0; i < (int)devices.size(); i++) {
      auto currDevice = devices[i];

      printDevice(currDevice);
      uint currMips = computeRubbishMips(currDevice);

      if(currMips > maxRubbishMips) {
        maxRubbishMips = currMips;
        maxRubbishMipsDevice = currDevice;
      }
    }
  }  
  cout << "Rubbish MIPs = CUs * max frequency (not necessarily accurate)\n\n";


  auto device = maxRubbishMipsDevice;
  string deviceName = device.getInfo<CL_DEVICE_NAME>();
  cout << fmt("Using device: \"%s\"\n", deviceName.c_str());

  auto context = cl::Context(device);
  cl::CommandQueue queue(context, device, 0, &err);
  clErr(err);

  return std::make_tuple(context, queue, device);
}

auto loadKernel(string path) -> string {
  std::ifstream in(path);
  
  if(!in) {
    cerr << fmt("Error loading kernel at: %s\n", path.c_str());
    std::abort();
  }

  string kernelSource = string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
  return kernelSource;
}

auto buildClCompileFlags(const vector<string>& includes) -> string {
  std::stringstream s;

  s << "-DOPENCL -cl-std=CL2.0 ";

  for (const auto& include : includes) {
    s << fmt("-I %s ", include.c_str());
  }

  return s.str();
}

auto buildProgram(
    string kernelFile,
    cl::Context &context,
    cl::Device& device,
    const vector<string>& includes
) -> cl::Program 
{
  string kernelSource = loadKernel(kernelFile);

  cl::Program::Sources source{kernelSource};
  cl::Program          program(context, source);

  string flags = buildClCompileFlags(includes);

  int err = program.build(device, flags.c_str());

  if (err == CL_BUILD_PROGRAM_FAILURE || err == CL_INVALID_PROGRAM) {
    cerr << fmt("Error building program for kernel: \"%s\"\n", kernelFile.c_str());

    // Get the build log
    string name     = device.getInfo<CL_DEVICE_NAME>();
    string buildlog = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device);

    cerr << fmt("Build log for: %s\n", name.c_str());
    cerr << buildlog;
  }

  return program;
}


auto kernelFromFile(
    std::string kernelPath,
    cl::Context& context,
    cl::Device& device,
    std::vector<std::string> includes
) -> cl::Kernel
{
  auto test_program = buildProgram(kernelPath, context, device, includes);
  

  std::filesystem::path filepath(kernelPath);
  
  cl_int err;
  cl::Kernel kernel(test_program, filepath.stem().c_str(), &err);
  clErr(err);

  if(err != CL_SUCCESS) {
    clErr(err);
    std::cerr << fmt("Failed to loa kernel at path: %s\n", kernelPath.c_str());
    std::abort();
  }

  return kernel;
}

float sqrt(float a) {
  return std::sqrt(a);
}

float length(float3 a) {
  float dx = a.s[0];
  float dy = a.s[1];
  float dz = a.s[2];

  return sqrt(dx*dx + dy*dy + dz*dz);
}

float dot(float3 a, float3 b) {
  return a.s[0] * b.s[0] + a.s[1] * b.s[1] + a.s[2] * b.s[2];
}

float3 normalize(float3 v) {
  return v / length(v);
}

float3 operator+(float3 a, float3 b) { 
  float3 c;

  c.s[0] = a.s[0] + b.s[0];  
  c.s[1] = a.s[1] + b.s[1];  
  c.s[2] = a.s[2] + b.s[2];  
  c.s[3] = 0;

  return c;
}

float3 operator-(float3 a, float3 b) { 
  float3 c;

  c.s[0] = a.s[0] - b.s[0];  
  c.s[1] = a.s[1] - b.s[1];  
  c.s[2] = a.s[2] - b.s[2];  
  c.s[3] = 0;

  return c;
}

float3 operator*(float3 a, float t) {
  float3 c;

  c.s[0] = a.s[0] * t;  
  c.s[1] = a.s[1] * t;  
  c.s[2] = a.s[2] * t;  
  c.s[3] = 0;

  return c;
}

float3 operator*(float t, float3 a) { return a * t; }

float3 operator/(float3 a, float t) {
  float3 c;

  c.s[0] = a.s[0] / t;  
  c.s[1] = a.s[1] / t;  
  c.s[2] = a.s[2] / t;  
  c.s[3] = 0;

  return c;
}

float3 operator-(float3 a) {
  float3 b;
  b.s[0] = -a.s[0];
  b.s[1] = -a.s[1];
  b.s[2] = -a.s[2];
  b.s[3] = 0;

  return b;
}

