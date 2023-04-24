#pragma once

#include "./CLErrors.h"
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

#define MATERIAL_DEF(Type, TypeTag, EqualityOpDef)                     \
  public:                                                              \
  inline static std::vector<Type> instances;                           \
                                                                       \
  static MaterialId push_back(Type mat) {                              \
    assert (mat.id.material_instance == 0);                            \
    auto matIter = std::find(instances.begin(), instances.end(), mat); \
                                                                       \
    if(matIter != instances.end()) {                                   \
      return matIter->id;                                              \
    }                                                                  \
                                                                       \
    return createAndPush(mat);                                         \
  }                                                                    \
                                                                       \
  EqualityOpDef                                                        \
                                                                       \
  private:                                                             \
  static MaterialId createAndPush(Type mat) {                          \
    mat.id.material_type = TypeTag;                                    \
    mat.id.material_instance = instances.size();                       \
    instances.push_back(mat);                                          \
    return mat.id;                                                     \
  }                                                                    
