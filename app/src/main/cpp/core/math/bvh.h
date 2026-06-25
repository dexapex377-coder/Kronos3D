#pragma once

#include "bounds.h"
#include <vector>

namespace kronos {

struct BVHNode {
    Bounds bounds;
    int left = -1;      // child index or primitive index (leaf)
    int right = -1;     // -1 for leaf
    int prim_count = 0; // >0 for leaf
    int prim_offset = 0;
};

struct BVH {
    std::vector<BVHNode> nodes;
    std::vector<int> prim_indices; // maps leaf prim_offset -> original primitive index

    void build(const std::vector<Bounds>& prim_bounds, int max_leaf_prims = 4) {
        int n = (int)prim_bounds.size();
        prim_indices.resize(n);
        for (int i = 0; i < n; ++i) prim_indices[i] = i;
        nodes.reserve(n * 2);
        build_recursive(prim_bounds, 0, n, max_leaf_prims);
    }

    int build_recursive(const std::vector<Bounds>& prim_bounds, int start, int end, int max_leaf) {
        int node_idx = (int)nodes.size();
        nodes.push_back({});
        BVHNode& node = nodes.back();

        node.prim_offset = start;
        node.prim_count = end - start;

        // Compute bounds
        node.bounds = Bounds::empty();
        for (int i = start; i < end; ++i)
            node.bounds.expand(prim_bounds[prim_indices[i]]);

        if (end - start <= max_leaf) {
            node.left = -1;
            node.right = -1;
            return node_idx;
        }

        // Choose split axis (largest extent)
        Vec3 extent = node.bounds.size();
        int axis = (extent.x > extent.y) ? (extent.x > extent.z ? 0 : 2) : (extent.y > extent.z ? 1 : 2);

        // Sort primitives by centroid on axis
        float split_pos = (node.bounds.min[axis] + node.bounds.max[axis]) * 0.5f;
        int mid = start;
        for (int i = start; i < end; ++i) {
            Vec3 c = prim_bounds[prim_indices[i]].center();
            if (c[axis] < split_pos) {
                std::swap(prim_indices[i], prim_indices[mid]);
                ++mid;
            }
        }

        // Fallback: split at middle if all on one side
        if (mid == start || mid == end) mid = start + (end - start) / 2;

        node.left = build_recursive(prim_bounds, start, mid, max_leaf);
        node.right = build_recursive(prim_bounds, mid, end, max_leaf);
        node.prim_count = 0;
        return node_idx;
    }

    // Ray-BVH traversal (stack-based, no recursion)
    template<typename HitFunc>
    void traverse(Vec3 ray_origin, Vec3 ray_dir, HitFunc&& hit_func) const {
        constexpr int MAX_STACK = 64;
        int stack[MAX_STACK];
        int stack_ptr = 0;
        stack[stack_ptr++] = 0; // root

        Vec3 inv_dir = {1.0f/ray_dir.x, 1.0f/ray_dir.y, 1.0f/ray_dir.z};
        int sign[3] = {inv_dir.x < 0, inv_dir.y < 0, inv_dir.z < 0};

        while (stack_ptr > 0) {
            int node_idx = stack[--stack_ptr];
            const BVHNode& node = nodes[node_idx];

            // Bounds check
            float tmin = -FLT_MAX, tmax = FLT_MAX;
            for (int i = 0; i < 3; ++i) {
                float o = (&ray_origin.x)[i];
                float inv_d = (&inv_dir.x)[i];
                float bmin = (&node.bounds.min.x)[i];
                float bmax = (&node.bounds.max.x)[i];
                float t1 = (bmin - o) * inv_d;
                float t2 = (bmax - o) * inv_d;
                if (sign[i]) std::swap(t1, t2);
                tmin = fmaxf(tmin, t1);
                tmax = fminf(tmax, t2);
                if (tmin > tmax) goto continue_traverse;
            }

            if (node.prim_count > 0) {
                // Leaf: test primitives
                for (int i = 0; i < node.prim_count; ++i) {
                    int prim_idx = prim_indices[node.prim_offset + i];
                    if (hit_func(prim_idx)) return; // early exit on hit
                }
            } else {
                // Internal: push children (near first)
                float t_left = FLT_MAX, t_right = FLT_MAX;
                if (node.left >= 0) {
                    const Bounds& b = nodes[node.left].bounds;
                    for (int i = 0; i < 3; ++i) {
                        float o = (&ray_origin.x)[i];
                        float inv_d = (&inv_dir.x)[i];
                        float t1 = (b.min[i] - o) * inv_d;
                        float t2 = (b.max[i] - o) * inv_d;
                        if (sign[i]) std::swap(t1, t2);
                        t_left = fmaxf(t_left, fminf(t1, t2));
                    }
                }
                if (node.right >= 0) {
                    const Bounds& b = nodes[node.right].bounds;
                    for (int i = 0; i < 3; ++i) {
                        float o = (&ray_origin.x)[i];
                        float inv_d = (&inv_dir.x)[i];
                        float t1 = (b.min[i] - o) * inv_d;
                        float t2 = (b.max[i] - o) * inv_d;
                        if (sign[i]) std::swap(t1, t2);
                        t_right = fmaxf(t_right, fminf(t1, t2));
                    }
                }
                if (t_left < t_right) {
                    if (node.right >= 0) stack[stack_ptr++] = node.right;
                    if (node.left >= 0) stack[stack_ptr++] = node.left;
                } else {
                    if (node.left >= 0) stack[stack_ptr++] = node.left;
                    if (node.right >= 0) stack[stack_ptr++] = node.right;
                }
            }
        continue_traverse:;
        }
    }
};

} // namespace kronos