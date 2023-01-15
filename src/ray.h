#pragma once
#include "vec3.h"

struct Ray {
	Vec3 o, d;

	Ray(Vec3 o, Vec3 d): o(o), d(d) {}
	Vec3 at(float t) const { return o + d * t; }
};

