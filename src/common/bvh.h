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
SHARED_STRUCT_START(BVHNode) {
	// sphere_count = 0 for non-leaf nodes, > 0 otherwise.
	uint left_node, first_sphere_index, sphere_count;
	AABB bounds;
} SHARED_STRUCT_END(BVHNode);

#ifdef OPENCL

#include "device/hit_record.h"
#include "device/interval.h"
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
			if(closest_hit(spheres + node->first_sphere_index, node->sphere_count, *ray, ray_t, &temp_rec)) {
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

#ifndef OPENCL 
class BVH {
	private:
		BVHNode* pool = nullptr;
		uint nodesUsed = 0;
		std::vector<Sphere>& spheres;

		const int rootNodeIndex = 0;
	public:
		BVH(std::vector<Sphere>& spheres)
			: spheres(spheres)
		{
			// No pool setup yet, preallocate 2N-1 Nodes.
			pool = static_cast<BVHNode*>(aligned_alloc(64, sizeof(BVHNode) * 2 * spheres.size()));
			std::memset(pool, 0, sizeof(BVHNode) * 2 * spheres.size());
			nodesUsed = 1;

			BVHNode& root = pool[rootNodeIndex];
			root.first_sphere_index = 0;
			root.sphere_count = spheres.size();
			root.left_node = nodesUsed;

			updateNodeBounds(rootNodeIndex);
			subdivide(rootNodeIndex);

			std::cout << "BVH built with " << nodesUsed << " Nodes.\n";
		}

		~BVH() {
			delete pool;
		}

		void updateNodeBounds(int node_index) {
			assert(pool != nullptr);

			BVHNode& node = pool[node_index];
			node.bounds.x = node.bounds.y = node.bounds.z = interval_empty();

			for(uint first = node.first_sphere_index, offset = 0; offset < node.sphere_count; offset++) {
				const AABB& sb = spheres[first + offset].bbox;
				AABB& nb = node.bounds;	
				nb.updateBounds(sb);
			}
		}

		void subdivide(int node_index) {
			BVHNode& node = pool[node_index];

			if(node.sphere_count <= 4) return;

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

			uint left_count = i - node.first_sphere_index;
			if(left_count == 0 || left_count == node.sphere_count) return;
			int left_child_index = nodesUsed++;
			int right_child_index = nodesUsed++;
			pool[left_child_index].first_sphere_index = node.first_sphere_index;
			pool[left_child_index].sphere_count = left_count;
			pool[right_child_index].first_sphere_index = i;
			pool[right_child_index].sphere_count = node.sphere_count - left_count;

			node.left_node = left_child_index;
			node.sphere_count = 0;

			updateNodeBounds(left_child_index);
			updateNodeBounds(right_child_index);

			subdivide(left_child_index);
			subdivide(right_child_index);
		}

		BVHNode* getPool() const { return pool; }
		const uint& getNodesUsed() const { return nodesUsed; }
};
#endif

