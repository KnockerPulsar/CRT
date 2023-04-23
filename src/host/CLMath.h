#include "host/CLUtil.h"

float3 f3(float x, float y, float z, float w);
float3 f3(float x, float y, float z);
float3 f3(float a);

float3 operator+(float3 a, float3 b);
float3 operator-(float3 a, float3 b);
float3 operator*(float3 a, float3 b);

float3 operator*(float3 a, float t);
float3 operator*(float t, float3 a);

float3 operator/(float3 a, float t);
float3 operator-(float3 a);

float length(float3 v);
float dot(float3 a, float3 b);
float3 normalize(float3 v);
float3 cross(float3 a, float3 b);

float degreesToRadians(float degrees);
