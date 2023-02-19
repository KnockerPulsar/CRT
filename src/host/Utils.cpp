#include "host/Utils.h"

auto replaceAll(const std::string& source, std::string toBeReplaced, std::string replacement) -> std::string {
  int pos;
  std::string sourceCopy = source;
  while ((size_t)(pos = sourceCopy.find(toBeReplaced)) != std::string::npos)
    sourceCopy.replace(pos, toBeReplaced.length(), replacement);

  return sourceCopy;
}

float clamp(float d, float min, float max) {
  const float t = d < min ? min : d;
  return t > max ? max : t;
}
