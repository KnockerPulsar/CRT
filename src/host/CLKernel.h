#pragma once

#include "CLUtil.h"

// Borrowed from https://github.com/ProjectPhysX/OpenCL-Wrapper/blob/master/src/opencl.hpp

// You should use this always.
// example: kernelParameters(kernel, 0, param1, param2, ...);
// The second parameter must _ALWAYS_ be 0.
// Recursively breaks down the given parameters to [param + the rest], uploads param, then recursively breaks the rest.
template<class T, class... U> inline void kernelParameters(cl_kernel kernel, const uint starting_position, const T& parameter, const U&... parameters) {
  kernelParameter(kernel, starting_position, parameter);
  kernelParameters(kernel, starting_position+1u, parameters...);
}

// Buffer arguments / cl_mem
template<typename T> inline void kernelParameter(cl_kernel kernel, const uint position, const CLBuffer<T>& memory) {
  clSetKernelArg(kernel, position, sizeof(cl_mem), &memory.devBuffer());
}

// Structs by value
template<typename T> inline void kernelParameter(cl_kernel kernel, const uint position, const T& value) {
  clSetKernelArg(kernel, position, sizeof(T), (const void*)&value);
}

// Base case
inline void kernelParameters(cl_kernel kernel, const uint starting_position) { }

