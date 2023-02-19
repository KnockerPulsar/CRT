#pragma once

// MUST DEFINE THESE TO AVOID COMPILATION ERRORS
#include "./CLErrors.h"

#define float3  cl_float3
#define float   cl_float
#define uint    cl_uint
#define uint2   cl_uint2
#define int2    cl_int2
#define u8      uint8_t 

#define constant const

#define pi       3.1415926535897932385

#include <iostream>
#include <fstream>
#include <optional>
#include <cmath>
#include <utility>

auto setupCL() -> std::tuple<cl::Context, cl::CommandQueue, cl::Device>; 
auto kernelFromFile( std::string kernelPath, cl::Context& context, cl::Device& devices, std::vector<std::string> includes = std::vector<std::string>()) -> cl::Kernel;

