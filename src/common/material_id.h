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

MaterialId mat_id_lambertian(uint instance) { return (MaterialId){MATERIAL_LAMBERTIAN, instance}; }
MaterialId mat_id_metal(uint instance) { return (MaterialId){MATERIAL_METAL, instance}; }
MaterialId mat_id_dielectric(uint instance) { return (MaterialId){MATERIAL_DIELECTRIC, instance}; }

