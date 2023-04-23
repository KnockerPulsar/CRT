#include "host/Utils.h"
#include "host/CLUtil.h"

auto replaceAll(const std::string& source, std::string toBeReplaced, std::string replacement) -> std::string {
  int pos;
  std::string sourceCopy = source;
  while ((size_t)(pos = sourceCopy.find(toBeReplaced)) != std::string::npos)
    sourceCopy.replace(pos, toBeReplaced.length(), replacement);

  return sourceCopy;
}

auto float3_equals(float3 a, float3 b) -> bool {
  return a.s[0] == b.s[0] 
    && a.s[1] && b.s[1]
    && a.s[2] && b.s[2];
}

auto material_id_equals(MaterialId a, MaterialId b) -> bool {
  return a.material_type == b.material_type 
    && a.material_instance == b.material_instance;
}
