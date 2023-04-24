#pragma once

#ifndef OPENCL
#include "host/CLUtil.h"
#endif

typedef struct {
	// texture_index holds the offset into the texture array that will define the texture type (solid color, checker, image)
	// which will affect how we'll sample each hit.
  uint material_type, material_instance, texture_index;
} MaterialId;

enum MaterialTypes {
	MATERIAL_LAMBERTIAN,
	MATERIAL_METAL,
	MATERIAL_DIELECTRIC
};
