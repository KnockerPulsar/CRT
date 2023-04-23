#include "host/CLMath.h"

#define FLOAT3_ELEMENTWISE_OP(a, b, op) \
  f3(                                   \
    (a).s[0] op (b).s[0],               \
    (a).s[1] op (b).s[1],               \
    (a).s[2] op (b).s[2]                \
  )                                     \

float3 f3(float x, float y, float z, float w) { return {{ x, y, z, w }}; }

float3 f3(float x, float y, float z) { return {{ x, y, z, 0}}; }

float3 operator+(float3 a, float3 b) { return FLOAT3_ELEMENTWISE_OP(a, b, +); }

float3 operator-(float3 a, float3 b) { return FLOAT3_ELEMENTWISE_OP(a, b, -); }

float3 operator*(float3 a, float3 b) { return FLOAT3_ELEMENTWISE_OP(a, b, *); }

float3 operator-(float3 a) { return f3(-a.s[0], -a.s[1], -a.s[2], 0); }


float3 operator*(float3 a, float t) { return FLOAT3_ELEMENTWISE_OP(a, f3(t, t, t, 0), *); }

float3 operator/(float3 a, float t) { return FLOAT3_ELEMENTWISE_OP(a, f3(t, t, t, 1), /); }
float3 operator/(float t, float3 a) { return FLOAT3_ELEMENTWISE_OP(a, f3(1/t, 1/t, 1/t, 1), /); }

float3 operator*(float t, float3 a) { return a * t; }

float sqrt(float a) { return std::sqrt(a); }

float length(float3 a) {
  float dx = a.s[0];
  float dy = a.s[1];
  float dz = a.s[2];

  return sqrt(dx*dx + dy*dy + dz*dz);
}

float dot(float3 a, float3 b) {
  return a.s[0] * b.s[0] + a.s[1] * b.s[1] + a.s[2] * b.s[2];
}

float3 normalize(float3 v) { return v / length(v); }

float3 cross(float3 a, float3 b) {
  float3 c;
  c.s[0] = a.s[1] * b.s[2] - a.s[2] * b.s[1];
  c.s[1] = a.s[2] * b.s[0] - a.s[0] * b.s[2];
  c.s[2] = a.s[0] * b.s[1] - a.s[1] * b.s[0];

  return c;
}

float degrees_to_radians(float degrees) { return degrees * pi / 180.0f; }
