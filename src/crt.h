#pragma once

#include <cmath>
#include <limits>
#include <memory>
#include <utility>

using std::shared_ptr;
using std::make_shared;
using std::sqrt;

const float infinity = std::numeric_limits<float>::infinity();
const float pi       =  3.1415926535897932385;

inline float degrees_to_radians(float degrees) {
	return degrees * pi / 180.0f;
}

float random_float() {
	return rand() / ((float)RAND_MAX + 1.0f);
	/* static std::uniform_real_distribution<double> distribution(0.0, 1.0); */
	/* static std::mt19937 generator; */
	/* return distribution(generator); */
}

float random_float(float min, float max) {
	return min + (max-min) * random_float();
}

#include "ray.h"
#include "vec3.h"

