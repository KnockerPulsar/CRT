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
using devMip = pair<cl::Device, int>;

template<typename T, typename U>
T getClInfo(U& platformOrDevice, cl_platform_info infoToGet) {
  T info;
  platformOrDevice.getInfo(infoToGet, &info);

  return info;
}

auto computeRubbishMips(cl::Device& device) -> uint {
    uint cus  = getClInfo<uint>(device, CL_DEVICE_MAX_COMPUTE_UNITS);
    uint freq = getClInfo<uint>(device, CL_DEVICE_MAX_CLOCK_FREQUENCY);

    return cus * freq;
}

auto getPlatformDevices(cl::Platform& platform) -> vector<cl::Device> {
  vector<cl::Device> platformDevices; 
  clErr(platform.getDevices(CL_DEVICE_TYPE_ALL, &platformDevices));

  return platformDevices;
}

auto printSize(const vector<uint>&& sizes) {
  std::stringstream ss;
  ss << "[ ";
  for (auto& item : sizes) {
    ss << item << ", ";
  }
  ss << " ]";
  return ss.str();
}

auto printDevice(cl::Device& device) {
    uint mips = computeRubbishMips(device);
    cout << "\tDevice name: " << getClInfo<string>(device, CL_DEVICE_NAME) << '\n';
    cout << "\tDevice CUs: " <<  getClInfo<uint>(device, CL_DEVICE_MAX_COMPUTE_UNITS) << '\n';
    cout << "\tDevice max frequency: " <<  getClInfo<uint>(device, CL_DEVICE_MAX_CLOCK_FREQUENCY) << '\n';
    cout << "\tDevice max workgroup size: " <<  printSize(getClInfo<vector<uint>>(device, CL_DEVICE_MAX_WORK_GROUP_SIZE)) << '\n';
    cout << "\tDevice max workitem size: " <<  printSize(getClInfo<vector<uint>>(device, CL_DEVICE_MAX_WORK_ITEM_SIZES)) << '\n';
    cout << "\tDevice max constant parameters: " <<  printSize(getClInfo<vector<uint>>(device, CL_DEVICE_MAX_CONSTANT_ARGS)) << '\n';
    cout << "\tDevice RubbishMIPs: " << mips << "\n\n";
}

auto getDevicesAndMips() -> vector<devMip> {
  vector<cl::Platform> platforms;
  clErr(cl::Platform::get(&platforms));

  vector<devMip> devicesAndMips;

  for(auto& platform : platforms) {
    cout << "\nPlatform name: " << getClInfo<string>(platform, CL_PLATFORM_NAME) << "\n";
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

auto setupCL() -> std::tuple<cl::Context, cl::CommandQueue, cl::Device> {

  auto devicesAndMips = getDevicesAndMips();
  auto maxElementIter = std::max_element(devicesAndMips.begin(), devicesAndMips.end(), 
      [](devMip a, devMip b) {
        return get<1>(a) < get<1>(b);
      } 
  );

  assert(maxElementIter != devicesAndMips.end() && "No OpenCL devices found");
  cl::Device device = std::get<0>(*maxElementIter);

  string deviceName = device.getInfo<CL_DEVICE_NAME>();
  cout << fmt("Using device: \"%s\"\n", deviceName.c_str());

  cl_int err;
  auto context = cl::Context(device);
  cl::CommandQueue queue(context, device, 0, &err);
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

  s << "-DOPENCL -cl-std=CL2.0 ";

  for (const auto& include : includes) {
    s << fmt("-I%s ", include.c_str());
  }

  std::cout << "Kernel compilation flags: " << std::quoted(s.str()) << std::endl;
  return s.str();
}

auto buildProgram(
    string kernelFile,
    cl::Context &context,
    cl::Device& device,
    const vector<string>& includes
) -> cl::Program 
{
  string kernelSource = loadKernel(kernelFile);

  cl::Program::Sources source{kernelSource};
  cl::Program          program(context, source);

  string flags = buildClCompileFlags(includes);

  int err = program.build(device, flags.c_str());

  if (err == CL_BUILD_PROGRAM_FAILURE || err == CL_INVALID_PROGRAM) {
    cerr << fmt("Error building program for kernel: \"%s\"\n", kernelFile.c_str());

    // Get the build log
    string name     = device.getInfo<CL_DEVICE_NAME>();
    string buildlog = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device);

    cerr << fmt("Build log for: %s\n", name.c_str());
    cerr << buildlog;
  }

  return program;
}


auto kernelFromFile(
    std::string kernelPath,
    cl::Context& context,
    cl::Device& device,
    std::vector<std::string> includes
) -> cl::Kernel
{
  auto test_program = buildProgram(kernelPath, context, device, includes);
  

  std::filesystem::path filepath(kernelPath);
  
  cl_int err;
  cl::Kernel kernel(test_program, filepath.stem().c_str(), &err);
  clErr(err);

  if(err != CL_SUCCESS) {
    clErr(err);
    std::cerr << fmt("Failed to loa kernel at path: %s\n", kernelPath.c_str());
    std::abort();
  }

  return kernel;
}
