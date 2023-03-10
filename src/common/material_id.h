#pragma once

#ifndef OPENCL
#include "host/CLUtil.h"
#endif

typedef struct { uint material_type, material_instance; } MaterialId;

enum MaterialTypes {
	MATERIAL_LAMBERTIAN,
	MATERIAL_METAL,
	MATERIAL_DIELECTRIC
};
