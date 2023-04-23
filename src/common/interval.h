#pragma once 

#include "device/cl_def.cl"

typedef struct {
	float min, max;
} Interval;

Interval interval_empty() { 
	return (Interval){+infinity, -infinity};
}

Interval interval_universe() { 
	return (Interval){-infinity, +infinity};
}

Interval interval(float min, float max) {
	return (Interval){min, max};
}

bool interval_contains(const Interval* i, float x) { return i->min <= x && x <= i->max; }

float interval_clamp(const Interval* i, float x) {
	if (x < i->min) return i->min;
	if (x > i->max) return i->max;

	return x;
}
