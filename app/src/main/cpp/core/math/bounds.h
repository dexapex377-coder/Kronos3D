#pragma once

#include "vector.h"

namespace kronos {

struct Bounds {
    Vec3 min, max;

    Bounds() : min{FLT_MAX, FLT_MAX, FLT_MAX}, max{-FLT_MAX, -FLT_MAX, -FLT_MAX} {}
    Bounds(Vec3 mn, Vec3 mx) : min(mn), max(mx) {}

    static Bounds empty() { return Bounds(); }

    void expand(Vec3 p) {
        min = kronos::min(min, p);
        max = kronos::max(max, p);
    }

    void expand(const Bounds& other) {
        if (other.is_empty()) return;
        if (is_empty()) { *this = other; return; }
        min = kronos::min(min, other.min);
        max = kronos::max(max, other.max);
    }

    bool is_empty() const { return min.x > max.x; }

    Vec3 center() const { return (min + max) * 0.5f; }
    Vec3 size() const { return max - min; }
    float surface_area() const {
        Vec3 s = size();
        return 2.0f * (s.x*s.y + s.y*s.z + s.z*s.x);
    }

    bool intersects(const Bounds& other) const {
        return min.x <= other.max.x && max.x >= other.min.x &&
               min.y <= other.max.y && max.y >= other.min.y &&
               min.z <= other.max.z && max.z >= other.min.z;
    }

    bool contains(Vec3 p) const {
        return p.x >= min.x && p.x <= max.x &&
               p.y >= min.y && p.y <= max.y &&
               p.z >= min.z && p.z <= max.z;
    }
};

inline Bounds transform_bounds(const Mat4& m, const Bounds& b) {
    if (b.is_empty()) return Bounds::empty();
    Vec3 corners[8] = {
        {b.min.x, b.min.y, b.min.z}, {b.max.x, b.min.y, b.min.z},
        {b.min.x, b.max.y, b.min.z}, {b.max.x, b.max.y, b.min.z},
        {b.min.x, b.min.y, b.max.z}, {b.max.x, b.min.y, b.max.z},
        {b.min.x, b.max.y, b.max.z}, {b.max.x, b.max.y, b.max.z}
    };
    Bounds r;
    for (int i = 0; i < 8; ++i) r.expand(transform_point(m, corners[i]));
    return r;
}

} // namespace kronos