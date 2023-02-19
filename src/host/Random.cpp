#include "CLMath.h"
#include "Random.h"

float random_float() { return rand() / (RAND_MAX + 1.0); }

float random_float_ranged(float min, float max) { return min + (max-min) * random_float(); }

float3 random_float3() { return f3(random_float(), random_float(), random_float()); }

float3 random_float3_ranged(float min, float max) {
  return f3( 
    random_float_ranged(min, min), 
    random_float_ranged(min, min), 
    random_float_ranged(min, min)
  );
}

