#include "cl_def.h"

float degrees_to_radians(float degrees) {
	return degrees * pi / 180.0f;
}

float random_float(uint2 seed1, uint seed2) {
	uint t = seed2 ^ (seed2 << 11);  
	uint result = seed1.x ^ (seed1.y >> 19) ^ (t ^ (t >> 8));

	return result;
}

float random_float_ranged(uint2 seed1, uint seed2, float min, float max) {
	return min + (max-min) * random_float(seed1, seed2);
}

