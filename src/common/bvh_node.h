#include "common/aabb.h"
#include "common/common_defs.h"
#include "common/sphere.h"

#ifndef OPENCL
#include "host/CLUtil.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <vector>
#endif

// https://jacco.ompf2.com/2022/06/03/how-to-build-a-bvh-part-9a-to-the-gpu/
#if 0
SHARED_STRUCT_START(TLASNode) {
	float3 min, max;
	uint left_right; // 2x16 bits	
	uint BLAS;
} SHARED_STRUCT_END(TLASNode);

SHARED_STRUCT_START(BVHNode) {
	float3 min, max;
	uint left_first; // 2x16 bits	
	uint tri_count;
} SHARED_STRUCT_END(BVHNode);

// Wraps/points to a BVH node
SHARED_STRUCT_START(BVHInstance) {
	uint idx;
	float16 transform, inverse_transform;
} SHARED_STRUCT_END(BVHInstance);

SHARED_STRUCT_START(Tri) { 
    float v0x, v0y, v0z;
    float v1x, v1y, v1z;
    float v2x, v2y, v2z;
    float cx, cy, cz;
} SHARED_STRUCT_END(Tri);


#else
SHARED_STRUCT_START(BVHNode) {
	// If non-leaf: Holds the index of the right child into the BVHNode list, the right child should be +1
	// If leaf: Holds the index of the first contained primitive (currently only spheres) into its respective array.
	// Nodes don't necessarily contain only one sphere, they can contain multiple ones.
	int left_node, first_sphere_index, sphere_count;
	AABB bounds;

#ifndef OPENCL
	BVHNode(std::vector<Sphere>& spheres, int& root_node_index, int& nodes_used, BVHNode** pool = nullptr) {
		// No pool setup yet, preallocate 2N-1 Nodes.
		if(nodes_used == 0)	{
			*pool = static_cast<BVHNode*>(aligned_alloc(64, sizeof(BVHNode) * 2 * spheres.size()));
			std::memset(*pool, 0, sizeof(BVHNode) * 2 * spheres.size());
			root_node_index = 0;
			nodes_used = 1;
		}

		BVHNode& root = *pool[root_node_index];
		root.first_sphere_index = 0;
		root.sphere_count = spheres.size();
		root.left_node = nodes_used;

		update_node_bounds(root_node_index, *pool, spheres);
		subdivide(root_node_index, *pool, spheres, nodes_used);
	}

	void update_node_bounds(int node_index, BVHNode* pool, const std::vector<Sphere>& spheres) {
		assert(pool != nullptr);

		BVHNode& node = pool[node_index];
		node.bounds.x = node.bounds.y = node.bounds.z = interval_empty();

		for(uint first = node.first_sphere_index, offset = 0; offset < node.sphere_count; offset++) {
			const AABB& sb = spheres[first + offset].bbox;
			AABB& nb = node.bounds;	
			nb.update_bounds(sb);
		}
	}

	void subdivide(int node_index, BVHNode* pool, std::vector<Sphere>& spheres, int& nodes_used) {
		BVHNode& node = pool[node_index];

		if(node.sphere_count <= 1) return;

		float3 extent = node.bounds.getExtent();

		int axis = 0;
		if(extent.s[1] > extent.s[0]) axis = 1;
		if(extent.s[2] > extent.s[axis]) axis = 2;
		float split_position = node.bounds.axis(axis).min + extent.s[axis] * 0.5f;

		int i = node.first_sphere_index;
		int j = i + node.sphere_count - 1;
		while(i <= j) {
			if(spheres[i].center.s[axis] < split_position) {
				i++;
			} else {
				std::swap(spheres[i], spheres[j]);
				j--;
			}
		}

		int left_count = i - node.first_sphere_index;
		if(left_count == 0 || left_count == node.sphere_count) return;
		int left_child_index = nodes_used++;
		int right_child_index = nodes_used++;
		pool[left_child_index].first_sphere_index = node.first_sphere_index;
		pool[left_child_index].sphere_count = left_count;
		pool[right_child_index].first_sphere_index = i;
		pool[right_child_index].sphere_count = node.sphere_count - left_count;

		node.left_node = left_child_index;
		node.sphere_count = 0;

		update_node_bounds(left_child_index, pool, spheres);
		update_node_bounds(right_child_index, pool, spheres);

		subdivide(left_child_index, pool, spheres, nodes_used);
		subdivide(right_child_index, pool, spheres, nodes_used);
	}
#endif
} SHARED_STRUCT_END(BVHNode);

#ifdef OPENCL

#include "device/hit_record.h"
#include "device/interval.h"
#include "device/ray.h"

bool bvh_intersect(global BVHNode* bvh_node, Ray* ray, Interval ray_t, HitRecord* rec, global Sphere* spheres) {
	global BVHNode* node = &bvh_node[0], *stack[32];
	uint stack_ptr = 0;

	bool hit_anything = false;
	HitRecord temp_rec;
	float closest_so_far = ray_t.max;

 if(node->left_node == 0 && node->first_sphere_index ==  0 && node->sphere_count == 0) return false;

#define POP_STACK \
			if(stack_ptr == 0) break; \
			else node = stack[--stack_ptr];

#define PUSH_STACK(value) \
	stack[stack_ptr++] = value;
	while(1) {
		
		// Leaf
		if(node->sphere_count > 0) {

			for(uint first = node->first_sphere_index, offset = 0; offset < node->sphere_count; offset++) {
				global Sphere* private s = &spheres[first + offset];
				if(sphere_hit(s, ray, interval(ray_t.min, closest_so_far), &temp_rec)) {

					closest_so_far = temp_rec.t;
					*rec = temp_rec;
					hit_anything = true;
				}
			}

			POP_STACK;
			continue;
		}

		global BVHNode* child1 = &bvh_node[node->left_node];
		global BVHNode* child2 = &bvh_node[node->left_node + 1];

		float dist1 = aabb_hit(&child1->bounds, ray, ray_t);
		float dist2 = aabb_hit(&child2->bounds, ray, ray_t);

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
#endif
