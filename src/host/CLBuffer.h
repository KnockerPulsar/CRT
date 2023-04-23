#pragma once

#include "CLUtil.h"
#include "host/CLErrors.h"
#include <cassert>
#include <vector>

template<class T> 
class CLBuffer {
  public:
    CLBuffer(cl_context& ctx, cl_command_queue& q, cl_mem_flags flags = CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, int count = 10)
      : hostBuffer(std::vector<T>()), queue(q), flags(flags)
    { 
      if(count > 0) {
        hostBuffer.reserve(count);
        /* hostBuffer.resize(count); */
      }
    }

    // https://stackoverflow.com/a/7278354
    static CLBuffer<T> fromPtr(cl_context& ctx, cl_command_queue& q, T* ptr, const int ptr_size) {
      CLBuffer<T> ret(ctx, q);

      // Makes a copy of the given array (ptr)
      std::vector<T> tempVec(ptr, ptr + ptr_size);
      ret.hostBuffer.swap(tempVec);

      return ret;
    }

    // Takes ownership of the given vector's data data
    static CLBuffer<T> fromVector(cl_context& ctx, cl_command_queue& q, std::vector<T>& vec) {
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

    void uploadToDevice(cl_context& ctx) {
      /* if(hostBuffer.data() == nullptr) return; */

      cl_int err;
      deviceBuffer = clCreateBuffer(ctx, flags | CL_MEM_COPY_HOST_PTR, hostBuffer.size() * sizeof(T), hostBuffer.data(), &err);
      clErr(err);
    }

    void readFromDevice() {
      clErr(clEnqueueReadBuffer(queue, deviceBuffer, CL_TRUE, 0, hostBuffer.size() * sizeof(T), hostBuffer.data(), 0, NULL, NULL));
    }

    const cl_mem& devBuffer() { return deviceBuffer; }
    const size_t count() const { return hostBuffer.size(); }

  private:
    std::vector<T> hostBuffer;
    cl_mem deviceBuffer;
    cl_command_queue queue;
    cl_mem_flags flags;
};
