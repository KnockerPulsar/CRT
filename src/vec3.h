#pragma once
#include <math.h>

struct Vec3 {
	float x, y, z;

	Vec3(float x, float y, float z) : x(x), y(y), z(z) { }

	Vec3 operator+(Vec3 r) const { return (Vec3) { x + r.x, y + r.y, z + r.z};  }
	Vec3 operator*(Vec3 r) const { return (Vec3) { x * r.x, y * r.y, z * r.z};  }
	Vec3 operator-(Vec3 r) const { return (Vec3) { x - r.x, y - r.y, z - r.z};  }
	Vec3 operator/(Vec3 r) const { return (Vec3) { x / r.x, y / r.y, z / r.z }; }

	Vec3 operator+(float r) { return (Vec3) { x + r, y + r, z + r }; }
	Vec3 operator-(float r) { return (Vec3) { x - r, y - r, z - r }; }
	Vec3 operator*(float r) { return (Vec3) { x * r, y * r, z * r }; }
	Vec3 operator/(float r) { return (Vec3) { x / r, y / r, z / r }; }

	float length() const { return sqrt(length_squared()); }
	float length_squared() const { return x * x + y * y + z * z; }
};

Vec3 unit_vector(Vec3 v) { return v / v.length(); }

Vec3 interpolate(Vec3 start, Vec3 end, float t) { return start * t + end * (1-t); }

float dot(Vec3 l, Vec3 r) { return l.x * r.x + l.y * r.y + l.z * r.z; }

Vec3 cross(Vec3 l, Vec3 r) {
	return Vec3(
			l.y * r.z - l.z * r.y,
			l.z * r.x - l.x * r.z,
			l.x * r.y - l.y * r.x
	);
}

Vec3 operator*(float f, Vec3 v) { return v * f; }
Vec3 operator/(float f, Vec3 v) { return v / f; }

