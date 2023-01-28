#include "CLUtil.h"

#include <CL/cl.h>
#include <cmath>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

using std::string, std::cout, std::vector, std::cerr, std::endl, std::optional;

cl::Context setupCL() {
  cl_int                    err;

  vector<cl::Platform> platformList;
  cl::Platform::get(&platformList);

  clErr(!platformList.empty() ? CL_SUCCESS : -1);

  for(int i = 0; i < (int)platformList.size(); i++) {
    string platformVendor;
    string platformVersion;
    string deviceVersion;
    string clCVersion;
    string deviceExtensions;

    platformList[i].getInfo((cl_platform_info)CL_PLATFORM_VENDOR, &platformVendor);
    platformList[i].getInfo((cl_platform_info)CL_PLATFORM_VERSION, &platformVersion);
    /* platformList[i].getInfo((cl_platform_info)CL_DEVICE_VERSION, &deviceVersion); */
    /* platformList[i].getInfo((cl_platform_info)CL_DEVICE_OPENCL_C_VERSION, &clCVersion); */
    platformList[i].getInfo((cl_platform_info)CL_DEVICE_EXTENSIONS, &deviceExtensions);
    cout << "Platform number is: " << i << endl;
    cout << "Platform is by: " << platformVendor << "\n";
    cout << "Platform version: " << platformVersion << "\n";
    /* cout << "Device version: " << deviceVersion << "\n"; */
    /* cout << "OpenCL C version: " << clCVersion << "\n"; */
    cout << "Device extensions: " << deviceExtensions << "\n";
  }

  cl_context_properties cprops[3] = {
    CL_CONTEXT_PLATFORM, (cl_context_properties)(platformList[0])(), 
    0 // To indicate list end
  };

  cl::Context context(CL_DEVICE_TYPE_GPU, cprops, NULL, NULL, &err);
  checkErr(err, "Context::Context()");

  return context;
}

auto loadKernel(string path) -> optional<string> {
  std::ifstream in(path);
  
  if(!in) {
    return std::nullopt;
  }

  string kernelSource = string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
  return make_optional(kernelSource);
}

auto build_cl_compile_flags(const vector<string>& includes) -> string {
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
) -> optional<cl::Program> 
{
  optional<string> kernelSource = loadKernel(kernelFile);

  if(!kernelSource.has_value()) { 
    cerr << "Error loading kernel at : " << kernelFile << endl;
    return std::nullopt;
  }

  cl::Program::Sources source{kernelSource.value()};
  cl::Program          program(context, source);

  string flags = build_cl_compile_flags(includes);

  int err = program.build(devices, flags.c_str());

  if (err == CL_BUILD_PROGRAM_FAILURE || err == CL_INVALID_PROGRAM) {
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

  return std::make_optional(program);
}

float sqrt(float a) {
  return sqrt(a);
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

