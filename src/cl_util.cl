#include "cl_def.cl"

float degrees_to_radians(float degrees) {
	return degrees * pi / 180.0f;
}

// https://stackoverflow.com/a/16077942
float random_float(uint2* seed) {
	const float invMaxInt = 1.0f/4294967296.0f;

	uint x = (*seed).x * 17 + (*seed).y * 13123;
	(*seed).x = (x<<13) ^ x;
	(*seed).y ^= (x<<7);

	uint tmp =  
		(x * (x * x * 15731 + 74323) + 871483) 
		+ (x * (x * x * 13734 + 37828) + 234234) 
		+ (x * (x * x * 11687 + 26461) + 137589);

	return convert_float(tmp) * invMaxInt;
}

float random_float_ranged(uint2* seed1, float min, float max) {
	return min + (max-min) * random_float(seed1);
}

