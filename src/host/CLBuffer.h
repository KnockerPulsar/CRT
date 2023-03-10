#pragma once

#include "CLUtil.h"
#include "host/CLErrors.h"
#include <CL/cl.h>
#include <CL/opencl.hpp>
#include <cassert>
#include <vector>

template<class T> 
class CLBuffer {
  public:
    CLBuffer(cl::Context& ctx, cl::CommandQueue& q, cl_mem_flags flags = CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, int count = 10)
      : hostBuffer(std::vector<T>()), queue(q), flags(flags)
    { 
      if(count > 0) {
        hostBuffer.reserve(count);
        /* hostBuffer.resize(count); */
      }
    }

    // Takes ownership of the given vector's data data
    static CLBuffer<T> fromVector(cl::Context& ctx, cl::CommandQueue& q, std::vector<T>& vec) {
      CLBuffer<T> ret(ctx, q);
      ret.hostBuffer.swap(vec);

      return ret;
    }

    CLBuffer& push_back(T e) {
      hostBuffer.push_back(e);
      return *this;
    }

    CLBuffer& push_back(std::vector<T> es) {
      for (auto& e : es) {
        push_back(e);
      }
      return *this;
    }

    T& operator[](int index) {
      return hostBuffer[index];
    }

    auto begin() -> typename std::vector<T>::iterator {
      return hostBuffer.begin();
    }

    auto end() -> typename std::vector<T>::iterator {
      return hostBuffer.end();
    }

    void uploadToDevice(cl::Context& ctx) {
      assert(hostBuffer.data() != nullptr);

      cl_int err;
      deviceBuffer = cl::Buffer(ctx, flags | CL_MEM_COPY_HOST_PTR, hostBuffer.size() * sizeof(T), hostBuffer.data(), &err);
      clErr(err);
    }

    void readFromDevice() {
      clErr(queue.enqueueReadBuffer(deviceBuffer, CL_TRUE, 0, hostBuffer.size() * sizeof(T), hostBuffer.data()));
    }

    cl::Buffer devBuffer() { return deviceBuffer; }
    int count() const { return hostBuffer.size(); }

  private:
    std::vector<T> hostBuffer;
    cl::Buffer deviceBuffer;
    cl::CommandQueue queue;
    cl_mem_flags flags;
};
