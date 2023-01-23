#include "CLUtil.h"
#include <CL/cl.h>
#include <optional>
#include <ostream>

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

auto buildProgram(std::string kernelFile, cl::Context &context, std::vector<cl::Device> devices) -> std::optional<cl::Program> {
  std::optional<std::string> kernelSource = loadKernel(kernelFile);

  if(!kernelSource.has_value()) return std::nullopt;

  cl::Program::Sources source{kernelSource.value()};
  cl::Program          program(context, source);

  int err = program.build(devices, "-DOPENCL -I src/common -I src");

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
