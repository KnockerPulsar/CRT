#include "host/CLUtil.h"
#include "host/Utils.h"

#include <CL/cl.h>
#include <algorithm>
#include <filesystem>
#include <iomanip>
#include <iterator>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <functional>

using std::string, std::cout, std::vector, std::cerr, std::endl, std::pair, std::make_pair, std::get;
using devMip = pair<cl_device_id, int>;

// Type alias for the signature of `clGetDeviceInfo` and `clGetPlatformInfo`.
template<typename T>
using infoFn = std::function<cl_int(T platDev, int param_name, 
    size_t param_value_size, void *param_value, size_t *param_value_size_ret)>;

// Used to check if the passed template parameter is a vector.
template<typename> struct is_std_vector : std::false_type {};
template<typename T, typename A> struct is_std_vector<std::vector<T,A>> : std::true_type {};

// For when you want to get an array of some type T from OpenCL.
// Mosty used to get strings (`char[]`) and `size_t[]`.
// fn is either `clGetDeviceInfo` or `clGetPlatformInfo`
template<typename T, typename U>
inline std::vector<T> getClArrInfo(U platDev, int infoToGet, infoFn<U> fn, uint defaultVecLen = 256) {
  std::vector<T> tVec(defaultVecLen, T{});                                                            
  size_t ret_len = 0;                                                                          

  clErr(fn(platDev, infoToGet, tVec.size() * sizeof(T), tVec.data(), &ret_len));

  assert(ret_len < (size_t)defaultVecLen);                                                                   
  tVec.resize(ret_len);                                                                     
  return tVec;
}

template<typename T, typename U> 
T getClInfo(U platform_or_device, int infoToGet);

// Overload for OpenCL strings for platforms.
template<>
inline std::string getClInfo(cl_platform_id platform, int infoToGet) {
   auto charVec = getClArrInfo<char, cl_platform_id>(platform, infoToGet, clGetPlatformInfo);
   return std::string(charVec.begin(), charVec.end());
}

// Overload for OpenCL strings for devices.
template <> 
inline std::string getClInfo(cl_device_id device, int infoToGet) {
   auto charVec = getClArrInfo<char, cl_device_id>(device, infoToGet, clGetDeviceInfo);
   return std::string(charVec.begin(), charVec.end());
}

// https://stackoverflow.com/a/14294277
// Only enabled if the passed type is numeric.
template<
    typename T, //real type
    typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type
>
inline T getClInfo(cl_device_id device, int infoToGet) {
  T ret; size_t ret_len = 0;
  clErr(clGetDeviceInfo(device, infoToGet, sizeof(T), &ret, &ret_len));
  assert(ret_len <= 8);

  return ret;
}

auto computeRubbishMips(cl_device_id device) -> uint {
    uint cus  = getClInfo<uint>(device, CL_DEVICE_MAX_COMPUTE_UNITS);
    uint freq = getClInfo<uint>(device, CL_DEVICE_MAX_CLOCK_FREQUENCY);

    return cus * freq;
}

auto getPlatformDevices(cl_platform_id& platform) -> vector<cl_device_id> {
  vector<cl_device_id> platformDevices(4);  // Up to 4 devices.
  uint num_devices = 0;
  clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, platformDevices.size(), platformDevices.data(), &num_devices);
  platformDevices.resize(num_devices);

  return platformDevices;
}

auto getPlatforms() -> vector<cl_platform_id> {
  const int maxPlatforms = 4;
  vector<cl_platform_id> platforms(maxPlatforms);
  cl_uint reported_num_platforms = 0;

  clGetPlatformIDs(maxPlatforms, platforms.data(), &reported_num_platforms);
  platforms.resize(reported_num_platforms);

  return platforms;
}

template<typename T>
auto printVector(const vector<T>&& sizes) {
  std::stringstream ss;
  ss << "[ "; for (const auto& item : sizes) { ss << item << ", "; } ss << " ]";
  return ss.str();
}

auto printDevice(cl_device_id& device) {
    uint mips = computeRubbishMips(device);
    cout 
      << "\tDevice name: " << getClInfo<std::string>(device, CL_DEVICE_NAME) << '\n'
      << "\tDevice CUs: " <<  getClInfo<uint>(device, CL_DEVICE_MAX_COMPUTE_UNITS) << '\n'
      << "\tDevice max frequency: " <<  getClInfo<uint>(device, CL_DEVICE_MAX_CLOCK_FREQUENCY) << '\n'
      << "\tDevice supports images: " <<  getClInfo<uint>(device, CL_DEVICE_IMAGE_SUPPORT) << '\n'
      << "\tDevice max workgroup size: " <<  getClInfo<size_t>(device, CL_DEVICE_MAX_WORK_GROUP_SIZE) << '\n'
      << "\tDevice max workitem size: " <<  printVector(getClArrInfo<size_t, cl_device_id>(device, CL_DEVICE_MAX_WORK_ITEM_SIZES, clGetDeviceInfo)) << '\n'
      << "\tDevice max constant parameters: " <<  getClInfo<uint>(device, CL_DEVICE_MAX_CONSTANT_ARGS) << '\n'
      << "\tDevice RubbishMIPs: " << mips << "\n\n";
}

auto getDevicesAndMips() -> vector<devMip> {
  auto platforms = getPlatforms();

  vector<devMip> devicesAndMips;

  for(auto& platform : platforms) {
    cout << "\nPlatform name: " << getClInfo<std::string>(platform, CL_PLATFORM_NAME) << "\n" << std::flush;
    /* string ext = getClInfo<string>(platform, CL_PLATFORM_EXTENSIONS);  */
    /* cout << "\nPlatform extensions: " << ext << "\n"; */

    for (auto& currDevice : getPlatformDevices(platform)) {
      printDevice(currDevice);
      uint currMips = computeRubbishMips(currDevice);
      devicesAndMips.emplace_back(make_pair(currDevice, currMips));
    }
  }  
  cout << "Rubbish MIPs = CUs * max frequency (not necessarily accurate)\n\n";

  return devicesAndMips;
}

auto setupCL() -> std::tuple<cl_context, cl_command_queue, cl_device_id> {

  auto devicesAndMips = getDevicesAndMips();
  auto maxElementIter = std::max_element(devicesAndMips.begin(), devicesAndMips.end(), 
      [](devMip a, devMip b) {
        return get<1>(a) < get<1>(b);
      } 
  );
  assert(maxElementIter != devicesAndMips.end() && "No OpenCL devices found");

  cl_device_id device = std::get<0>(*maxElementIter);

  auto deviceName = getClInfo<std::string>(device, CL_DEVICE_NAME);
  cout << fmt("Using device: \"%s\"\n", deviceName.c_str());

  cl_int err;
  cl_context context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
  cl_command_queue queue = clCreateCommandQueueWithProperties(context, device, NULL, &err);
  clErr(err);

  return std::make_tuple(context, queue, device);
}

auto loadKernel(string path) -> string {
  std::ifstream in(path);
  
  if(!in) {
    cerr << fmt("Error loading kernel at: %s\n", path.c_str());
    std::abort();
  }

  return string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

auto buildClCompileFlags(const vector<string>& includes) -> string {
  std::stringstream s;

  s << "-DOPENCL -cl-std=CL2.0 ";

#ifdef DEBUG
  s << " -g -cl-opt-disable ";
#else
  s << " -cl-fast-relaxed-math ";
#endif

  for (const auto& include : includes) {
    s << fmt("-I%s ", include.c_str());
  }

  std::cout << "Kernel compilation flags: " << std::quoted(s.str()) << std::endl;
  return s.str();
}

auto buildProgram(
    string kernelFile,
    cl_context &context,
    cl_device_id& device,
    const vector<string>& includes
) -> cl_program
{
  string kernelSource = loadKernel(kernelFile);

  int err;
  const char* sources[] = {kernelSource.c_str()};
  const size_t sizes[] = {kernelSource.size()};
  cl_program program = clCreateProgramWithSource(context, 1, sources, sizes, &err);
  clErr(err);

  string flags = buildClCompileFlags(includes);

  err = clBuildProgram(program, 1, &device, flags.c_str(), NULL, NULL);

  if (err == CL_BUILD_PROGRAM_FAILURE || err == CL_INVALID_PROGRAM) {
    cerr << fmt("Error building program for kernel: \"%s\"\n", kernelFile.c_str());

    // Get the build log
    std::string name = getClInfo<std::string>(device, CL_DEVICE_NAME);

    char str[8192];
    size_t ret_len = 0;
    clErr(clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 8192, str, &ret_len));
    string buildlog = std::string(str);

    cerr << fmt("Build log for: %s\n", name.c_str());
    cerr << buildlog;
  }

  return program;
}


auto kernelFromFile(
    std::string kernelPath,
    cl_context& context,
    cl_device_id& device,
    std::vector<std::string> includes
) -> cl_kernel
{
  cl_program test_program = buildProgram(kernelPath, context, device, includes);

  std::filesystem::path filepath(kernelPath);
  
  cl_int err;
  cl_kernel kernel = clCreateKernel(test_program, filepath.stem().c_str(), &err);
  clErr(err);

  if(err != CL_SUCCESS) {
    clErr(err);
    std::cerr << fmt("Failed to loa kernel at path: %s\n", kernelPath.c_str());
    std::abort();
  }

  return kernel;
}
