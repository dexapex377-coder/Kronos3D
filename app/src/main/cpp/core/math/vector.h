#pragma once

#include "blenlib.h"

namespace kronos {

struct float2_t { float x, y; };
struct float3_t { float x, y, z; };
struct float4_t { float x, y, z, w; };

using Vec2 = float2_t;
using Vec3 = float3_t;
using Vec4 = float4_t;

inline Vec2 make_vec2(float x, float y) { return {x, y}; }
inline Vec3 make_vec3(float x, float y, float z) { return {x, y, z}; }
inline Vec4 make_vec4(float x, float y, float z, float w) { return {x, y, z, w}; }

inline Vec2 operator+(Vec2 a, Vec2 b) { return {a.x + b.x, a.y + b.y}; }
inline Vec3 operator+(Vec3 a, Vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline Vec4 operator+(Vec4 a, Vec4 b) { return {a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w}; }

inline Vec2 operator-(Vec2 a, Vec2 b) { return {a.x - b.x, a.y - b.y}; }
inline Vec3 operator-(Vec3 a, Vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline Vec4 operator-(Vec4 a, Vec4 b) { return {a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w}; }

inline Vec2 operator*(Vec2 a, float s) { return {a.x * s, a.y * s}; }
inline Vec3 operator*(Vec3 a, float s) { return {a.x * s, a.y * s, a.z * s}; }
inline Vec4 operator*(Vec4 a, float s) { return {a.x * s, a.y * s, a.z * s, a.w * s}; }

inline Vec2 operator/(Vec2 a, float s) { return {a.x / s, a.y / s}; }
inline Vec3 operator/(Vec3 a, float s) { return {a.x / s, a.y / s, a.z / s}; }
inline Vec4 operator/(Vec4 a, float s) { return {a.x / s, a.y / s, a.z / s, a.w / s}; }

inline float dot(Vec2 a, Vec2 b) { return a.x * b.x + a.y * b.y; }
inline float dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline float dot(Vec4 a, Vec4 b) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }

inline Vec3 cross(Vec3 a, Vec3 b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

inline float length_sq(Vec2 a) { return dot(a, a); }
inline float length_sq(Vec3 a) { return dot(a, a); }
inline float length_sq(Vec4 a) { return dot(a, a); }

inline float length(Vec2 a) { return sqrtf(length_sq(a)); }
inline float length(Vec3 a) { return sqrtf(length_sq(a)); }
inline float length(Vec4 a) { return sqrtf(length_sq(a)); }

inline Vec2 normalize(Vec2 a) { float l = length(a); return l > 0 ? a / l : Vec2{0, 0}; }
inline Vec3 normalize(Vec3 a) { float l = length(a); return l > 0 ? a / l : Vec3{0, 0, 0}; }
inline Vec4 normalize(Vec4 a) { float l = length(a); return l > 0 ? a / l : Vec4{0, 0, 0, 0}; }

inline Vec3 reflect(Vec3 v, Vec3 n) { return v - n * (2.0f * dot(v, n)); }
inline Vec3 refract(Vec3 v, Vec3 n, float eta) {
    float cosi = clamp(-dot(v, n), -1.0f, 1.0f);
    float k = 1.0f - eta * eta * (1.0f - cosi * cosi);
    return k < 0.0f ? Vec3{0, 0, 0} : v * eta + n * (eta * cosi - sqrtf(k));
}

inline Vec3 faceforward(Vec3 n, Vec3 i, Vec3 nref) {
    return dot(nref, i) < 0.0f ? n : Vec3{-n.x, -n.y, -n.z};
}

inline Vec3 mix(Vec3 a, Vec3 b, float t) { return a + (b - a) * t; }
inline Vec3 min(Vec3 a, Vec3 b) { return {fminf(a.x, b.x), fminf(a.y, b.y), fminf(a.z, b.z)}; }
inline Vec3 max(Vec3 a, Vec3 b) { return {fmaxf(a.x, b.x), fmaxf(a.y, b.y), fmaxf(a.z, b.z)}; }
inline Vec3 clamp(Vec3 v, Vec3 mn, Vec3 mx) { return max(min(v, mx), mn); }

} // namespace kronos