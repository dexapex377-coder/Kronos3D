#pragma once

#include "vector.h"

namespace kronos {

struct Mat3 {
    float m[3][3];

    static Mat3 identity() {
        return {{{1,0,0}, {0,1,0}, {0,0,1}}};
    }
};

struct Mat4 {
    float m[4][4];

    static Mat4 identity() {
        return {{{1,0,0,0}, {0,1,0,0}, {0,0,1,0}, {0,0,0,1}}};
    }

    static Mat4 translation(Vec3 t) {
        Mat4 r = identity();
        r.m[0][3] = t.x; r.m[1][3] = t.y; r.m[2][3] = t.z;
        return r;
    }

    static Mat4 scale(Vec3 s) {
        Mat4 r = identity();
        r.m[0][0] = s.x; r.m[1][1] = s.y; r.m[2][2] = s.z;
        return r;
    }

    static Mat4 rotation_x(float angle) {
        float c = cosf(angle), s = sinf(angle);
        return {{{1,0,0,0}, {0,c,-s,0}, {0,s,c,0}, {0,0,0,1}}};
    }

    static Mat4 rotation_y(float angle) {
        float c = cosf(angle), s = sinf(angle);
        return {{{c,0,s,0}, {0,1,0,0}, {-s,0,c,0}, {0,0,0,1}}};
    }

    static Mat4 rotation_z(float angle) {
        float c = cosf(angle), s = sinf(angle);
        return {{{c,-s,0,0}, {s,c,0,0}, {0,0,1,0}, {0,0,0,1}}};
    }

    static Mat4 perspective(float fovy, float aspect, float near, float far) {
        float f = 1.0f / tanf(fovy * 0.5f);
        Mat4 r = {{{0}}};
        r.m[0][0] = f / aspect;
        r.m[1][1] = f;
        r.m[2][2] = (far + near) / (near - far);
        r.m[2][3] = (2.0f * far * near) / (near - far);
        r.m[3][2] = -1.0f;
        return r;
    }

    static Mat4 look_at(Vec3 eye, Vec3 target, Vec3 up) {
        Vec3 f = normalize(target - eye);
        Vec3 s = normalize(cross(f, up));
        Vec3 u = cross(s, f);
        Mat4 r = identity();
        r.m[0][0] = s.x; r.m[0][1] = s.y; r.m[0][2] = s.z;
        r.m[1][0] = u.x; r.m[1][1] = u.y; r.m[1][2] = u.z;
        r.m[2][0] = -f.x; r.m[2][1] = -f.y; r.m[2][2] = -f.z;
        r.m[0][3] = -dot(s, eye);
        r.m[1][3] = -dot(u, eye);
        r.m[2][3] = dot(f, eye);
        return r;
    }
};

inline Mat4 operator*(const Mat4& a, const Mat4& b) {
    Mat4 r = {{{0}}};
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            for (int k = 0; k < 4; ++k)
                r.m[i][j] += a.m[i][k] * b.m[k][j];
    return r;
}

inline Vec4 operator*(const Mat4& m, const Vec4& v) {
    return {
        m.m[0][0]*v.x + m.m[0][1]*v.y + m.m[0][2]*v.z + m.m[0][3]*v.w,
        m.m[1][0]*v.x + m.m[1][1]*v.y + m.m[1][2]*v.z + m.m[1][3]*v.w,
        m.m[2][0]*v.x + m.m[2][1]*v.y + m.m[2][2]*v.z + m.m[2][3]*v.w,
        m.m[3][0]*v.x + m.m[3][1]*v.y + m.m[3][2]*v.z + m.m[3][3]*v.w
    };
}

inline Vec3 transform_point(const Mat4& m, const Vec3& v) {
    Vec4 r = m * make_vec4(v.x, v.y, v.z, 1.0f);
    return {r.x / r.w, r.y / r.w, r.z / r.w};
}

inline Vec3 transform_vector(const Mat4& m, const Vec3& v) {
    Vec4 r = m * make_vec4(v.x, v.y, v.z, 0.0f);
    return {r.x, r.y, r.z};
}

inline Mat4 inverse(const Mat4& m) {
    // Simplified for affine transforms (rotation + translation + uniform scale)
    Mat4 r = {{{0}}};
    // Transpose 3x3 rotation part
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            r.m[i][j] = m.m[j][i];
    // Inverse translation
    Vec3 t = {m.m[0][3], m.m[1][3], m.m[2][3]};
    Vec3 it = transform_vector(r, t);
    r.m[0][3] = -it.x; r.m[1][3] = -it.y; r.m[2][3] = -it.z;
    r.m[3][3] = 1.0f;
    return r;
}

struct Quat { float x, y, z, w; };

inline Quat quat_identity() { return {0, 0, 0, 1}; }

inline Quat quat_from_axis_angle(Vec3 axis, float angle) {
    float half = angle * 0.5f;
    float s = sinf(half);
    axis = normalize(axis);
    return {axis.x * s, axis.y * s, axis.z * s, cosf(half)};
}

inline Quat quat_mul(Quat a, Quat b) {
    return {
        a.w*b.x + a.x*b.w + a.y*b.z - a.z*b.y,
        a.w*b.y - a.x*b.z + a.y*b.w + a.z*b.x,
        a.w*b.z + a.x*b.y - a.y*b.x + a.z*b.w,
        a.w*b.w - a.x*b.x - a.y*b.y - a.z*b.z
    };
}

inline Vec3 quat_rotate(Quat q, Vec3 v) {
    Vec3 u = {q.x, q.y, q.z};
    float s = q.w;
    Vec3 t = cross(u, v) * 2.0f;
    return v + t * s + cross(u, t);
}

inline Mat4 quat_to_mat4(Quat q) {
    float xx = q.x*q.x, yy = q.y*q.y, zz = q.z*q.z;
    float xy = q.x*q.y, xz = q.x*q.z, yz = q.y*q.z;
    float wx = q.w*q.x, wy = q.w*q.y, wz = q.w*q.z;
    return {{{1-2*(yy+zz), 2*(xy-wz), 2*(xz+wy), 0},
             {2*(xy+wz), 1-2*(xx+zz), 2*(yz-wx), 0},
             {2*(xz-wy), 2*(yz+wx), 1-2*(xx+yy), 0},
             {0, 0, 0, 1}}};
}

} // namespace kronos