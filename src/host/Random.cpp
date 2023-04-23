#include "CLMath.h"
#include "Random.h"

float randomFloat() { return rand() / (RAND_MAX + 1.0); }

float randomFloatRanged(float min, float max) { return min + (max-min) * randomFloat(); }

float3 randomFloat3() { return f3(randomFloat(), randomFloat(), randomFloat()); }

float3 randomFloat3Ranged(float min, float max) {
  return f3( 
    randomFloatRanged(min, min), 
    randomFloatRanged(min, min), 
    randomFloatRanged(min, min)
  );
}

