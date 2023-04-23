#pragma once

#include "./CLErrors.h"
#include "host/CLBuffer.h"
#include <vector>

#define float3  cl_float3
#define float   cl_float
#define uint    cl_uint
#define uint2   cl_uint2
#define int2    cl_int2
#define u8      uint8_t 
#define float16 cl_float16

#define constant const

#define pi       3.1415926535897932385

#include <iostream>
#include <fstream>
#include <optional>
#include <cmath>
#include <utility>

auto setupCL() -> std::tuple<cl_context, cl_command_queue, cl_device_id>; 
auto kernelFromFile( std::string kernelPath, cl_context& context, cl_device_id& devices, std::vector<std::string> includes = std::vector<std::string>()) -> cl_kernel;

// Borrowed from https://github.com/ProjectPhysX/OpenCL-Wrapper/blob/master/src/opencl.hpp
template<typename T> inline void kernel_parameter(cl_kernel kernel, const uint position, const CLBuffer<T>& memory) {
  clSetKernelArg(kernel, position, sizeof(cl_mem), &memory.devBuffer());
}

template<typename T> inline void kernel_parameter(cl_kernel kernel, const uint position, const T& value) {
  clSetKernelArg(kernel, position, sizeof(T), (const void*)&value);
}

inline void kernel_parameters(cl_kernel kernel, const uint starting_position) { }

template<class T, class... U> inline void kernel_parameters(cl_kernel kernel, const uint starting_position, const T& parameter, const U&... parameters) {
  kernel_parameter(kernel, starting_position, parameter);
  kernel_parameters(kernel, starting_position+1u, parameters...);
}
