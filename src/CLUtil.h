#pragma once

// MUST DEFINE THESE TO AVOID COMPILATION ERRORS
#define CL_TARGET_OPENCL_VERSION 220
#define CL_HPP_TARGET_OPENCL_VERSION 220

#ifdef OPENCL 
#else 
  #include <CL/opencl.hpp>
  #define float3 cl_float3
  #define float cl_float
#endif


#include <iostream>
#include <fstream>
#include <optional>

cl::Context setupCL(); 
auto loadKernel(std::string path) -> std::optional<std::string>;
auto buildProgram(std::string kernelFile, cl::Context &context, std::vector<cl::Device> devices) -> std::optional<cl::Program>;

inline void checkErr(cl_int err, const char *name) {
  if (err != CL_SUCCESS) {
    std::cerr << "ERROR: " << name << " (" << err << ") " << std::endl;
    exit(EXIT_FAILURE);
  }
}

#define clErr(err) checkErr(err, ("FILE: " + std::string(__FILE__) + ", LINE: " + std::to_string(__LINE__)).c_str())
