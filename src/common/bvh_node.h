#include "common/aabb.h"
#include "common/common_defs.h"
#include "common/sphere.h"
#include "common/interval.h"

#ifndef OPENCL
#include "host/CLUtil.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <vector>
#endif

// https://jacco.ompf2.com/2022/06/03/how-to-build-a-bvh-part-9a-to-the-gpu/
SHARED_STRUCT_START(BVHNode) {
	// sphere_count = 0 for non-leaf nodes, > 0 otherwise.
	uint left_node, first_sphere_index, sphere_count;
	AABB bounds;
} SHARED_STRUCT_END(BVHNode);

#ifdef OPENCL

#include "device/hit_record.h"
#include "device/ray.h"
#include "device/cl_util.cl"

bool bvh_intersect(global BVHNode* bvh_node, global Sphere* spheres, Ray* ray, Interval* ray_t, HitRecord* rec) {
	global BVHNode* node = &bvh_node[0], *stack[32];
	uint stack_ptr = 0;

	bool hit_anything = false;
	HitRecord temp_rec;

 if(node->left_node == 0 && node->first_sphere_index ==  0 && node->sphere_count == 0) return false;

#define POP_STACK \
			if(stack_ptr == 0) break; \
			else node = stack[--stack_ptr];

#define PUSH_STACK(value) \
	stack[stack_ptr++] = value;

	while(1) {
		
		// Leaf
		if(node->sphere_count > 0) {
#if 1
			if(closest_hit(spheres + node->first_sphere_index, node->sphere_count, *ray , ray_t, &temp_rec)) {
				*rec = temp_rec;
				hit_anything = true;
			}
#else
			for(uint first = node->first_sphere_index, offset = 0; offset < node->sphere_count; offset++) {
				global Sphere* private s = &spheres[first + offset];
				if(sphere_hit(s, ray, interval(ray_t.min, closest_so_far), &temp_rec)) {

					closest_so_far = temp_rec.t;
					*rec = temp_rec;
					hit_anything = true;
				}
			}
#endif
			POP_STACK;
			continue;
		}

		global BVHNode* child1 = &bvh_node[node->left_node];
		global BVHNode* child2 = &bvh_node[node->left_node + 1];

		float dist1 = aabb_hit(&child1->bounds, ray, *ray_t);
		float dist2 = aabb_hit(&child2->bounds, ray, *ray_t);

		if(dist1 > dist2) {
				SWAP(dist1, dist2, float);
				SWAP(child1, child2, global BVHNode*);
		}

		if(dist1 >= infinity) {
			POP_STACK;
		} else {
			node = child1;
			if(dist2 < infinity) PUSH_STACK(child2);
		}
	}
	return hit_anything;
}
#endif
