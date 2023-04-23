#pragma once

#include <vector>

#include "common/bvh_node.h"
#include "common/sphere.h"

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
			// Tried 32, 64, 128, 256. All perform similarly on Vega 6.
			pool = static_cast<BVHNode*>(aligned_alloc(64, sizeof(BVHNode) * 2 * spheres.size()));

			// GCC complains about memset...
			for(size_t i = 0; i < 2 * spheres.size(); i++) { pool[i] = {0}; }


			nodesUsed = 1;
			BVHNode& root = pool[rootNodeIndex];
			root.sphere_count = spheres.size();
			root.left_first = 0;

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

			for(uint first = node.left_first, offset = 0; offset < node.sphere_count; offset++) {
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

			int i = node.left_first;
			int j = i + node.sphere_count - 1;
			while(i <= j) {
				if(spheres[i].center.s[axis] < split_position) {
					i++;
				} else {
					std::swap(spheres[i], spheres[j]);
					j--;
				}
			}

			uint left_count = i - node.left_first;
			if(left_count == 0 || left_count == node.sphere_count) return;
			int left_child_index = nodesUsed++;
			int right_child_index = nodesUsed++;
			pool[left_child_index].left_first = node.left_first;
			pool[left_child_index].sphere_count = left_count;
			pool[right_child_index].left_first = i;
			pool[right_child_index].sphere_count = node.sphere_count - left_count;

			node.left_first = left_child_index;
			node.sphere_count = 0;

			updateNodeBounds(left_child_index);
			updateNodeBounds(right_child_index);

			subdivide(left_child_index);
			subdivide(right_child_index);
		}

		BVHNode* getPool() const { return pool; }
		const uint& getNodesUsed() const { return nodesUsed; }
};
