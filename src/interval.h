#pragma once 

#include "crt.h"
struct Interval {
	float min, max;
	
	Interval(): min(+infinity), max(-infinity) {}
	Interval(float min, float max): min(min), max(max) {}

	bool contains(float x) { return min <= x && x <= max; }

	float clamp(float x) {
		if (x < min) return min;
		if (x > max) return max;

		return x;
	}

	const static Interval empty, universe;
};

const static Interval empty		(+infinity, -infinity);
const static Interval universe(-infinity, +infinity);
