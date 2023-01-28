#include "CLUtil.h"
#include <CL/cl.h>
#include <cmath>
#include <optional>
#include <ostream>
#include <sstream>

cl::Context setupCL() {
  cl_int                    err;

  std::vector<cl::Platform> platformList;
  cl::Platform::get(&platformList);

  clErr(!platformList.empty() ? CL_SUCCESS : -1);



  for(int i = 0; i < (int)platformList.size(); i++) {
    std::string platformVendor;

    platformList[i].getInfo((cl_platform_info)CL_PLATFORM_VENDOR, &platformVendor);
    std::cerr << "Platform number is: " << i << std::endl;
    std::cerr << "Platform is by: " << platformVendor << "\n";
  }

  cl_context_properties cprops[3] = {
    CL_CONTEXT_PLATFORM, (cl_context_properties)(platformList[0])(), 
    0 // To indicate list end
  };

  cl::Context context(CL_DEVICE_TYPE_GPU, cprops, NULL, NULL, &err);
  checkErr(err, "Context::Context()");

  return context;
}

auto loadKernel(std::string path) -> std::optional<std::string> {
  std::ifstream in(path);
  
  if(!in) {
    return std::nullopt;
  }

  std::string kernelSource = std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
  return std::make_optional(kernelSource);
}

auto build_cl_compile_flags(const std::vector<std::string>& includes) -> std::string {
  std::stringstream s;

  s << "-DOPENCL ";

  for (const auto& include : includes) {
    s << "-I " <<  include << " ";
  }

  return s.str();
}

auto buildProgram(
    std::string kernelFile,
    cl::Context &context,
    std::vector<cl::Device> devices,
    const std::vector<std::string>& includes
) -> std::optional<cl::Program> 
{
  std::optional<std::string> kernelSource = loadKernel(kernelFile);

  if(!kernelSource.has_value()) return std::nullopt;

  cl::Program::Sources source{kernelSource.value()};
  cl::Program          program(context, source);

  std::string flags = build_cl_compile_flags(includes);

  int err = program.build(devices, flags.c_str());

  if (err == CL_BUILD_PROGRAM_FAILURE || err == CL_INVALID_PROGRAM) {
    for (const cl::Device &dev : devices) {

      // Check the build status
      cl_build_status status = program.getBuildInfo<CL_PROGRAM_BUILD_STATUS>(dev);
      if (status != CL_BUILD_ERROR)
        continue;

      // Get the build log
      std::string name     = dev.getInfo<CL_DEVICE_NAME>();
      std::string buildlog = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(dev);
      std::cerr << "Build log for " << name << ":" << std::endl << buildlog << std::endl;
    }
  }

  return std::make_optional(program);
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

