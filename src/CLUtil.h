#pragma once

// MUST DEFINE THESE TO AVOID COMPILATION ERRORS
#define CL_TARGET_OPENCL_VERSION 220
#define CL_HPP_TARGET_OPENCL_VERSION 220


#ifndef OPENCL
#include <CL/cl.h>
#include <CL/cl_platform.h>
#include <CL/opencl.hpp>

#define float3 cl_float3
#define float  cl_float

#define uint  cl_uint
#define uint2 cl_uint2
#define int2  cl_int2

#define constant const

#include <iostream>
#include <fstream>
#include <optional>
#include <cmath>
#include <utility>

#include "cl_errors.h"


  auto setupCL() -> std::tuple<cl::Context, cl::CommandQueue, cl::Device>; 
  auto loadKernel(std::string path) -> std::string;
  auto buildClCompileFlags(const std::string& includes) -> std::string;
  auto buildProgram(std::string kernelFile, cl::Context &context, cl::Device& devices, const std::vector<std::string>& includes) -> cl::Program;
  auto kernelFromFile( std::string kernelPath, cl::Context& context, cl::Device& devices, std::vector<std::string> includes = std::vector<std::string>()) -> cl::Kernel;

  inline void checkErr(cl_int err, const char *name) {
    if (err != CL_SUCCESS) {
      std::cerr << "ERROR: " << name << " (" << clErrorString(err) << ") " << std::endl;
      exit(EXIT_FAILURE);
    }
  }

#define clErr(err) checkErr(err, ("FILE: " + std::string(__FILE__) + ", LINE: " + std::to_string(__LINE__)).c_str())


  float length(float3 v);
  float dot(float3 a, float3 b);
  float3 normalize(float3 v);

  float3 operator+(float3 a, float3 b);
  float3 operator-(float3 a, float3 b);
  float3 operator*(float3 a, float t);
  float3 operator*(float t, float3 a);
  float3 operator/(float3 a, float t);
  float3 operator-(float3 a);
#endif



/* 
 * To be able to use a struct in both OpenCL and C++ comfortably.
 *
 *
 * Example:
 * SHARED_STRUCT_START(Foo) {
 *  ... // Foo impl.
 * } SHARED_STRUCT_END(Foo);
 * 
 *
 * Expands to (on OpenCL's side):
 * typedef struct {
 *  ... // Foo impl.
 * } Foo;
 * 
 *
 * Or on C++'s side:
 * struct Foo {
 *  ... // Foo impl.
 * };
 */
#ifdef OPENCL 

#define SHARED_STRUCT_START(s) typedef struct 
#define SHARED_STRUCT_END(s) s

#else 

#define SHARED_STRUCT_START(s) struct s 
#define SHARED_STRUCT_END(s) // Nothing

#endif
#ifdef OPENCL 

#define SHARED_STRUCT_START(s) typedef struct s 
#define SHARED_STRUCT_END(s) s

#else 

#define SHARED_STRUCT_START(s) struct s 
#define SHARED_STRUCT_END(s) // Nothing

#endif
