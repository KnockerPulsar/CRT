#include "CLUtil.h"

#include <CL/cl.h>
#include <CL/cl_ext.h>
#include <CL/cl_platform.h>
#include <CL/opencl.hpp>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iterator>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>

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

auto getDevices(vector<cl::Platform> platforms) {
  vector<cl::Device> devices;

  for (auto& platform: platforms) {
    vector<cl::Device> platformDevices; 
    platform.getDevices(CL_DEVICE_TYPE_ALL, &platformDevices);

    devices.insert(devices.end(), platformDevices.begin(), platformDevices.end());
  } 

  return devices;
}

auto setupCL() -> std::tuple<cl::Context, cl::CommandQueue, vector<cl::Device>> {
  cl_int err;

  vector<cl::Platform> platforms;
  cl::Platform::get(&platforms);

  vector<cl::Device> devices = getDevices(platforms);

  cl_context_properties cprops[3] = {
    CL_CONTEXT_PLATFORM, 
    (cl_context_properties)(platforms[1])(), 
    0 // To indicate list end
  };


  cl::Context context(devices[1], cprops, NULL, NULL, &err);
  cl::CommandQueue queue(context, 0, &err);

  return make_tuple(context, queue, devices);
}

auto loadKernel(string path) -> string {
  std::ifstream in(path);
  
  if(!in) {
    cerr << "Error loading kernel at : " << path << endl;
    std::abort();
  }

  string kernelSource = string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
  return kernelSource;
}

auto buildClCompileFlags(const vector<string>& includes) -> string {
  std::stringstream s;

  s << "-DOPENCL ";
  s << "-cl-std=CL2.0 ";

  for (const auto& include : includes) {
    s << "-I " <<  include << " ";
  }

  return s.str();
}

auto buildProgram(
    string kernelFile,
    cl::Context &context,
    vector<cl::Device> devices,
    const vector<string>& includes
) -> cl::Program 
{
  string kernelSource = loadKernel(kernelFile);

  cl::Program::Sources source{kernelSource};
  cl::Program          program(context, source);

  string flags = buildClCompileFlags(includes);

  int err = program.build(devices, flags.c_str());

  if (err == CL_BUILD_PROGRAM_FAILURE || err == CL_INVALID_PROGRAM) {
    cerr << "Error building program for kernel: " << kernelFile << std::endl;

    for (const cl::Device &dev : devices) {
      // Check the build status
      cl_build_status status = program.getBuildInfo<CL_PROGRAM_BUILD_STATUS>(dev);
      if (status != CL_BUILD_ERROR)
        continue;

      // Get the build log
      string name     = dev.getInfo<CL_DEVICE_NAME>();
      string buildlog = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(dev);
      cerr << "Build log for " << name << ":" << endl << buildlog << endl;
    }
  }

  return program;
}


auto kernelFromFile(
    std::string kernelPath,
    cl::Context& context,
    std::vector<cl::Device> devices,
    std::vector<std::string> includes
) -> cl::Kernel
{
  auto test_program = buildProgram(kernelPath, context, devices, includes);
  

  std::filesystem::path filepath(kernelPath);
  
  cl_int err;
  cl::Kernel kernel(test_program, filepath.stem().c_str(), &err);
  auto dev = devices[1];
  string name     = dev.getInfo<CL_DEVICE_NAME>();
  string buildlog = test_program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(dev);
  cerr << "Build log for " << name << ":" << endl << buildlog << endl;
  cerr << "Kernel: " << kernelPath << '\n';
  clErr(err);

  if(err != CL_SUCCESS) {
    clErr(err);
    std::cerr << "Failed to load kernel at: " << kernelPath << std::endl;
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

