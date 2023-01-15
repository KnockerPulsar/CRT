#pragma once

#include <stdint.h>
#define u8 uint8_t 

float clamp(float d, float min, float max) {
  const float t = d < min ? min : d;
  return t > max ? max : t;
}
