#pragma once

#include "../math/vector.h"
#include "../math/bounds.h"
#include <cstdint>

namespace kronos {
namespace cycles {

// Kernel data structures (ported from Blender 4.5 kernel_types.h)

struct KernelGlobals {
    // Scene data
    int width, height;
    int samples;
    int max_bounces;
    int diffuse_bounces;
    int glossy_bounces;
    int transmission_bounces;
    int volume_bounces;
    int transparent_bounces;

    // Camera
    float3 camera_p;
    float3 camera_d;
    float3 camera_u;
    float3 camera_v;
    float camera_aperture;
    float camera_focal;
    float camera_fstop;
    float camera_focus_distance;

    // Film
    float exposure;
    float gamma;

    // Background
    int background_type; // 0=sky, 1=texture
    float3 background_color;

    // BVH
    int bvh_root;
    int bvh_nodes_offset;
    int bvh_primitives_offset;

    // Lights
    int num_lights;
    int lights_offset;

    // Mesh data
    int num_triangles;
    int triangles_offset;
    int vertices_offset;
    int normals_offset;
    int uvs_offset;

    // Textures
    int num_textures;
    int textures_offset;
    int texture_data_offset;

    // RNG
    uint64_t seed;
};

struct Ray {
    float3 P;
    float3 D;
    float tmin, tmax;
    float time;
    int depth;
    int flags;
};

struct Intersection {
    float t;
    float u, v;
    int prim_idx;
    int object_idx;
    int shader_idx;
    bool hit;
};

struct ShaderData {
    float3 N;
    float3 Ng;
    float3 P;
    float3 V;
    float3 dPdu, dPdv;
    float u, v;
    int shader_id;
    int object_id;
    int prim_id;
};

struct LightSample {
    float3 P;
    float3 N;
    float3 Le;
    float pdf;
    int light_id;
};

struct PathState {
    Ray ray;
    float3 throughput;
    int depth;
    int bounce;
    bool terminated;
    uint32_t rng_state;
};

// Kernel math functions (NEON optimized)
inline float3 kernel_ray_intersect_triangle(const Ray& ray, const float3& v0, const float3& v1, const float3& v2, float& u, float& v) {
    float3 e1 = v1 - v0;
    float3 e2 = v2 - v0;
    float3 h = cross(ray.D, e2);
    float a = dot(e1, h);
    if (fabsf(a) < 1e-8f) return float3{0,0,0}; // parallel

    float f = 1.0f / a;
    float3 s = ray.P - v0;
    u = f * dot(s, h);
    if (u < 0.0f || u > 1.0f) return float3{0,0,0};

    float3 q = cross(s, e1);
    v = f * dot(ray.D, q);
    if (v < 0.0f || u + v > 1.0f) return float3{0,0,0};

    float t = f * dot(e2, q);
    if (t < ray.tmin || t > ray.tmax) return float3{0,0,0};

    return float3{t, u, v};
}

// Möller-Trumbore intersection
inline bool intersect_triangle(const Ray& ray, const float3& v0, const float3& v1, const float3& v2, float& t, float& u, float& v) {
    float3 edge1 = v1 - v0;
    float3 edge2 = v2 - v0;
    float3 h = cross(ray.D, edge2);
    float a = dot(edge1, h);

    if (fabsf(a) < 1e-8f) return false;
    float f = 1.0f / a;

    float3 s = ray.P - v0;
    u = f * dot(s, h);
    if (u < 0.0f || u > 1.0f) return false;

    float3 q = cross(s, edge1);
    v = f * dot(ray.D, q);
    if (v < 0.0f || u + v > 1.0f) return false;

    t = f * dot(edge2, q);
    return t >= ray.tmin && t <= ray.tmax;
}

// BVH traversal (ported from kernel_bvh.h)
struct BVHNode {
    float3 bounds_min[2];
    float3 bounds_max[2];
    int children[2]; // leaf: prim_offset, prim_count in children[0]
    int prim_offset;
    int prim_count;
    int axis;
    bool is_leaf() const { return prim_count > 0; }
};

inline bool bvh_node_intersect(const Ray& ray, const BVHNode& node, float& tmin, float& tmax) {
    float3 invD = {1.0f/ray.D.x, 1.0f/ray.D.y, 1.0f/ray.D.z};
    int sign[3] = {invD.x < 0, invD.y < 0, invD.z < 0};

    float t1 = (node.bounds_min[sign[0]].x - ray.P.x) * invD.x;
    float t2 = (node.bounds_max[sign[0]].x - ray.P.x) * invD.x;
    tmin = fmaxf(tmin, fminf(t1, t2));
    tmax = fminf(tmax, fmaxf(t1, t2));
    if (tmin > tmax) return false;

    t1 = (node.bounds_min[sign[1]].y - ray.P.y) * invD.y;
    t2 = (node.bounds_max[sign[1]].y - ray.P.y) * invD.y;
    tmin = fmaxf(tmin, fminf(t1, t2));
    tmax = fminf(tmax, fmaxf(t1, t2));
    if (tmin > tmax) return false;

    t1 = (node.bounds_min[sign[2]].z - ray.P.z) * invD.z;
    t2 = (node.bounds_max[sign[2]].z - ray.P.z) * invD.z;
    tmin = fmaxf(tmin, fminf(t1, t2));
    tmax = fminf(tmax, fmaxf(t1, t2));
    return tmin <= tmax;
}

// Path tracing kernel (simplified from kernel_path_integrate.h)
inline float3 path_trace(KernelGlobals* kg, Ray ray, uint32_t& rng_state) {
    float3 throughput = {1.0f, 1.0f, 1.0f};
    float3 radiance = {0.0f, 0.0f, 0.0f};

    for (int bounce = 0; bounce < kg->max_bounces; ++bounce) {
        // BVH traversal
        Intersection isect = {FLT_MAX, 0, 0, -1, -1, -1, false};
        // ... traverse BVH, find closest intersection

        if (!isect.hit) {
            // Background
            radiance = radiance + throughput * kg->background_color;
            break;
        }

        // Evaluate shader
        ShaderData sd;
        // ... fill shader data from intersection

        // Direct lighting
        // ... sample lights, add to radiance

        // BSDF sampling
        // ... sample BSDF, update ray, throughput

        // Russian roulette
        if (bounce > 3) {
            float p = fmaxf(throughput.x, fmaxf(throughput.y, throughput.z));
            if (p < 0.05f) break;
            float rr = 1.0f - p;
            uint32_t r = rng_state * 1664525 + 1013904223;
            rng_state = r;
            float xi = (float)r / (float)UINT32_MAX;
            if (xi < rr) break;
            throughput = throughput / (1.0f - rr);
        }
    }

    return radiance;
}

} // namespace cycles
} // namespace kronos