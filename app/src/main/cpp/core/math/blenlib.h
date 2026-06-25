#pragma once

#include <cstdint>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <limits>

#ifdef ANDROID
#include <android/log.h>
#define KRONOS_LOG_TAG "Kronos3D"
#define KRONOS_LOGI(...) __android_log_print(ANDROID_LOG_INFO, KRONOS_LOG_TAG, __VA_ARGS__)
#define KRONOS_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, KRONOS_LOG_TAG, __VA_ARGS__)
#else
#define KRONOS_LOGI(...) printf(__VA_ARGS__)
#define KRONOS_LOGE(...) fprintf(stderr, __VA_ARGS__)
#endif

namespace kronos {

using float2 = float[2];
using float3 = float[3];
using float4 = float[4];
using int2 = int[2];
using int3 = int[3];
using int4 = int[4];

constexpr float PI = 3.14159265358979323846f;
constexpr float DEG2RAD = PI / 180.0f;
constexpr float RAD2DEG = 180.0f / PI;
constexpr float FLT_EPSILON = 1.192092896e-07f;

inline float square(float x) { return x * x; }
inline float clamp(float x, float min, float max) { return x < min ? min : (x > max ? max : x); }
inline float lerp(float a, float b, float t) { return a + (b - a) * t; }
inline float smoothstep(float edge0, float edge1, float x) {
    float t = clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

inline void swap(float& a, float& b) { float t = a; a = b; b = t; }
inline void swap(int& a, int& b) { int t = a; a = b; b = t; }

} // namespace kronos