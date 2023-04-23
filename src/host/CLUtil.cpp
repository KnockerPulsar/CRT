#include "host/CLUtil.h"
#include "host/Utils.h"

#include <algorithm>
#include <iterator>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>
#include <filesystem>

#include <assert.h>
#include <cmath>
#include <cstdlib>

using std::string, std::cout, std::vector, std::cerr, std::endl, std::pair, std::make_pair, std::get;
using devMip = pair<cl_device_id, int>;

std::string getPlatformInfoStr(cl_platform_id& platform , cl_platform_info infoToGet) {
  char str[128];
  size_t ret_len = 0;
  clErr(clGetPlatformInfo(platform, infoToGet, 128, str, &ret_len));
  assert(ret_len < 128);

  return std::string(str);
}

uint getPlatformInfoUint(cl_platform_id& platform , cl_platform_info infoToGet) {
  uint ret;
  size_t ret_len = 0;
  clErr(clGetPlatformInfo(platform, infoToGet, 1, &ret, &ret_len));
  assert(ret_len < 4);

  return ret;
}

std::string getDeviceInfoStr(cl_device_id& device, cl_platform_info infoToGet) {
  char str[1024];
  size_t ret_len = 0;
  clErr(clGetDeviceInfo(device, infoToGet, 1024, str, &ret_len));
  assert(ret_len < 1024);

  return std::string(str);
}

uint getDeviceInfoUint(cl_device_id& device , cl_platform_info infoToGet) {
  uint ret;
  size_t ret_len = 0;
  clErr(clGetDeviceInfo(device, infoToGet, sizeof(uint), &ret, &ret_len));
  assert(ret_len < 16);

  return ret;
}

auto computeRubbishMips(cl_device_id device) -> uint {
    uint cus  = getDeviceInfoUint(device, CL_DEVICE_MAX_COMPUTE_UNITS);
    uint freq = getDeviceInfoUint(device, CL_DEVICE_MAX_CLOCK_FREQUENCY);

    return cus * freq;
}

auto getPlatformDevices(cl_platform_id& platform) -> vector<cl_device_id> {
  vector<cl_device_id> platformDevices(4); 
  uint num_devices = 0;
  clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, platformDevices.size(), platformDevices.data(), &num_devices);
  platformDevices.resize(num_devices);

  return platformDevices;
}

auto printSize(const vector<uint>&& sizes) {
  std::stringstream ss;
  ss << "[ ";
  for (const auto& item : sizes) {
    ss << item << ", ";
  }
  ss << " ]";
  return ss.str();
}

auto printDevice(cl_device_id& device) {
    uint mips = computeRubbishMips(device);
    cout << "\tDevice name: " << getDeviceInfoStr(device, CL_DEVICE_NAME) << '\n';
    cout << "\tDevice CUs: " <<  getDeviceInfoUint(device, CL_DEVICE_MAX_COMPUTE_UNITS) << '\n';
    cout << "\tDevice max frequency: " <<  getDeviceInfoUint(device, CL_DEVICE_MAX_CLOCK_FREQUENCY) << '\n';
    cout << "\tDevice supports images: " <<  getDeviceInfoUint(device, CL_DEVICE_IMAGE_SUPPORT) << '\n';
    /* cout << "\tDevice max workgroup size: " <<  printSize(getClInfo<vector<uint>>(device, CL_DEVICE_MAX_WORK_GROUP_SIZE)) << '\n'; */
    /* cout << "\tDevice max workitem size: " <<  printSize(getClInfo<vector<uint>>(device, CL_DEVICE_MAX_WORK_ITEM_SIZES)) << '\n'; */
    /* cout << "\tDevice max constant parameters: " <<  printSize(getClInfo<vector<uint>>(device, CL_DEVICE_MAX_CONSTANT_ARGS)) << '\n'; */
    cout << "\tDevice RubbishMIPs: " << mips << "\n\n";
}

auto getDevicesAndMips() -> vector<devMip> {
  vector<cl_platform_id> platforms(4);

  cl_uint num_platforms = 0;
  clGetPlatformIDs(4, platforms.data(), &num_platforms);
  platforms.resize(num_platforms);

  vector<devMip> devicesAndMips;

  for(auto& platform : platforms) {
    cout << "\nPlatform name: " << getPlatformInfoStr(platform, CL_PLATFORM_NAME) << "\n" << std::flush;
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

/* extern cl_int clGetDeviceInfo(cl_device_id device, cl_device_info param_name, */
/*                               size_t param_value_size, void *param_value, */
/*                               size_t *param_value_size_ret) */

  char deviceName[128];
  size_t returnedNameLength = 0;
  clGetDeviceInfo(device, CL_DEVICE_NAME, 128, deviceName, &returnedNameLength);
  cout << fmt("Using device: \"%s\"\n", deviceName);

  cl_int err;
  auto context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);


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

  string kernelSource = string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
  return kernelSource;
}

auto buildClCompileFlags(const vector<string>& includes) -> string {
  std::stringstream s;

  s << "-DOPENCL -cl-std=CL2.0";

#ifdef DEBUG
  const std::string debugFlags = " -g ";
  std::cout << "Building kernels with debug flags (" << debugFlags << ")\n";
  s << debugFlags;
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
    string name = getDeviceInfoStr(device, CL_DEVICE_NAME);

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
  auto test_program = buildProgram(kernelPath, context, device, includes);
  

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
