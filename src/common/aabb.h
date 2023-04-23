#pragma once

#ifndef OPENCL
#include "host/CLMath.h"
#endif

#include "common/common_defs.h"
#include "common/interval.h"

SHARED_STRUCT_START(AABB) {
	Interval x, y, z;

#ifndef OPENCL
	AABB(): x(interval_empty()), y(interval_empty()), z(interval_empty()) {}

	AABB(const Interval& ix, const Interval& iy, const Interval& iz): 
		x(ix), y(iy), z(iz) {}

	AABB(const float3& a, const float3& b) {
		x = interval(fminf(a.s[0], b.s[0]), fmaxf(a.s[0], b.s[0]));
		y = interval(fminf(a.s[1], b.s[1]), fmaxf(a.s[1], b.s[1]));
		z = interval(fminf(a.s[2], b.s[2]), fmaxf(a.s[2], b.s[2]));
	}

	const Interval& axis(int n) const {
		if (n==1) return y;
		if (n==2) return z;
		return x; 
	}

	void grow(const AABB& other) {
			x.min = fminf(x.min, other.x.min);
			x.max = fmaxf(x.max, other.x.max);

			y.min = fminf(y.min, other.y.min);
			y.max = fmaxf(y.max, other.y.max);

			z.min = fminf(z.min, other.z.min);
			z.max = fmaxf(z.max, other.z.max);
	}

	float3 getExtent() const {
		return f3(
				x.max - x.min,
				y.max - y.min,
				z.max - z.min
				);
	}

	float area() const {
		float3 a = getExtent();
		return a.s[0] * a.s[1] 
					+ a.s[1] * a.s[2] 
					+ a.s[2] * a.s[0];
	}
#endif
} SHARED_STRUCT_END(AABB);

#ifdef OPENCL
#include "device/ray.h"
#include "device/cl_util.cl"

const Interval axis(global const AABB* aabb, int n) {
	if (n==1) return aabb->y;
	if (n==2) return aabb->z;
	return aabb->x;
}

#define comp(vec, idx)   	\
	((idx == 0? vec.x : ((idx == 1)? vec.y : vec.z)))

float aabb_hit(global const AABB* aabb, const Ray* r, Interval ray_t) {
#if 1
	// https://jacco.ompf2.com/2022/04/13/how-to-build-a-bvh-part-1-basics/
	float tx1 = (aabb->x.min - r->o.x) / r->d.x, tx2 = (aabb->x.max - r->o.x) / r->d.x;
	float tmin = min( tx1, tx2 ), tmax = max( tx1, tx2 );

	float ty1 = (aabb->y.min - r->o.y) / r->d.y, ty2 = (aabb->y.max - r->o.y) / r->d.y;
	tmin = max( tmin, min( ty1, ty2 ) ), tmax = min( tmax, max( ty1, ty2 ) );

	float tz1 = (aabb->z.min - r->o.z) / r->d.z, tz2 = (aabb->z.max - r->o.z) / r->d.z;
	tmin = max( tmin, min( tz1, tz2 ) ), tmax = min( tmax, max( tz1, tz2 ) );

	if (tmax >= tmin && tmin < ray_t.max && tmax > 0) return tmin; else return infinity;
#else
	// Adapted from "Raytracing in one weekend"
	// A bit slower.
	float tmin = ray_t.min, tmax = ray_t.max;
	for (int a = 0; a < 3; a++) {
		float inv_d = 1.0f / comp(r->d, a);

		float t0 = (axis(aabb, a).min - comp(r->o, a)) * inv_d;
		float t1 = (axis(aabb, a).max - comp(r->o, a)) * inv_d;

		if (inv_d < 0.0f)
			SWAP(t0, t1, float);

		tmin = t0 > tmin ? t0 : tmin;
		tmax = t1 < tmax ? t1 : tmax;

		if (tmax <= tmin)
			return infinity;
	}
	return tmin;
#endif
}
#endif
