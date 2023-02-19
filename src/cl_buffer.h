#include "CLUtil.h"
#include <cassert>
#include <vector>

template<class T> 
class CLBuffer {
  public:
    CLBuffer(cl::Context& ctx, cl::CommandQueue& q, cl_mem_flags flags = CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, int count = 10)
      : hostBuffer(std::vector<T>()), deviceBuffer(cl::Buffer(ctx, flags, count * sizeof(T), hostBuffer.data())), queue(q) 
    { 
      if(count > 0) {
        hostBuffer.reserve(count);
        /* hostBuffer.resize(count); */
      }
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

    void uploadToDevice() {
      assert(hostBuffer.data() != nullptr);
      clErr(queue.enqueueWriteBuffer(deviceBuffer, CL_TRUE, 0, hostBuffer.size() * sizeof(T), hostBuffer.data()));
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
};
