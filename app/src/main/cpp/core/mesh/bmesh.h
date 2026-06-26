#pragma once

#include "../math/vector.h"
#include "../math/bounds.h"
#include <vector>
#include <cstdint>

namespace kronos {

// BMesh topology (ported from Blender's BMESH)
struct BMVert {
    Vec3 co;           // coordinates
    Vec3 no;           // normal
    int flag = 0;      // BM_ELEM_SELECT, BM_ELEM_HIDDEN, etc.
    int index = -1;    // runtime index
    void* data = nullptr; // custom data (UV, color, etc.)
};

struct BMEdge {
    BMVert* vert[2] = {nullptr, nullptr};
    int flag = 0;
    int index = -1;
    void* data = nullptr;
};

struct BMLoop {
    BMVert* vert = nullptr;
    BMEdge* edge = nullptr;
    struct BMFace* face = nullptr;
    BMLoop* next = nullptr;
    BMLoop* prev = nullptr;
    BMLoop* radial_next = nullptr;
    BMLoop* radial_prev = nullptr;
    float uv[2] = {0, 0};
    int flag = 0;
    int index = -1;
};

struct BMFace {
    BMLoop* loops = nullptr;
    int len = 0;
    Vec3 no;           // face normal
    Vec3 cent;         // centroid
    int flag = 0;
    int index = -1;
    void* data = nullptr;
    int mat_nr = 0;    // material index
};

struct BMesh {
    std::vector<BMVert> verts;
    std::vector<BMEdge> edges;
    std::vector<BMFace> faces;
    std::vector<BMLoop> loops;

    Bounds bounds;

    // Element flags
    enum Flag : int {
        SELECT = 1 << 0,
        HIDDEN = 1 << 1,
        TAG    = 1 << 2,
        BOUNDARY = 1 << 3,
        NON_MANIFOLD = 1 << 4,
    };

    void clear() {
        verts.clear(); edges.clear(); faces.clear(); loops.clear();
        bounds = Bounds::empty();
    }

    void ensure_lookup_tables() {
        for (size_t i = 0; i < verts.size(); ++i) verts[i].index = (int)i;
        for (size_t i = 0; i < edges.size(); ++i) edges[i].index = (int)i;
        for (size_t i = 0; i < faces.size(); ++i) faces[i].index = (int)i;
        for (size_t i = 0; i < loops.size(); ++i) loops[i].index = (int)i;
    }

    void calc_bounds() {
        bounds = Bounds::empty();
        for (auto& v : verts) bounds.expand(v.co);
    }

    void calc_normals() {
        // Face normals
        for (auto& f : faces) {
            if (f.len < 3) { f.no = {0,0,0}; continue; }
            Vec3 n = {0,0,0};
            Vec3 v0 = f.loops->vert->co;
            Vec3 v1 = f.loops->next->vert->co;
            Vec3 v2 = f.loops->next->next->vert->co;
            n = cross(v1 - v0, v2 - v0);
            f.no = normalize(n);
        }
        // Vertex normals (average of face normals)
        for (auto& v : verts) v.no = {0,0,0};
        for (auto& f : faces) {
            if (f.len < 3) continue;
            BMLoop* l = f.loops;
            for (int i = 0; i < f.len; ++i, l = l->next) {
                l->vert->no = l->vert->no + f.no;
            }
        }
        for (auto& v : verts) {
            float len = length(v.no);
            if (len > 0) v.no = v.no / len;
        }
    }

    // Create primitive cube
    static BMesh cube(float size = 1.0f) {
        BMesh bm;
        bm.verts.reserve(8);
        bm.loops.reserve(24);
        bm.faces.reserve(6);
        float h = size * 0.5f;
        Vec3 coords[8] = {
            {-h,-h,-h}, {h,-h,-h}, {h,h,-h}, {-h,h,-h},
            {-h,-h,h},  {h,-h,h},  {h,h,h},  {-h,h,h}
        };
        for (int i = 0; i < 8; ++i) {
            BMVert v; v.co = coords[i]; bm.verts.push_back(v);
        }
        int face_verts[6][4] = {
            {0,3,2,1}, {4,5,6,7}, {0,1,5,4},
            {1,2,6,5}, {2,3,7,6}, {3,0,4,7}
        };
        for (int fi = 0; fi < 6; ++fi) {
            BMFace f; f.len = 4; f.mat_nr = 0;
            BMLoop* first = nullptr; BMLoop* prev = nullptr;
            for (int li = 0; li < 4; ++li) {
                BMLoop l; l.vert = &bm.verts[face_verts[fi][li]];
                l.face = &f;
                bm.loops.push_back(l);
                BMLoop* cur = &bm.loops.back();
                if (prev) { prev->next = cur; cur->prev = prev; }
                else first = cur;
                prev = cur;
            }
            prev->next = first; first->prev = prev;
            f.loops = first;
            bm.faces.push_back(f);
        }
        bm.ensure_lookup_tables();
        bm.calc_normals();
        bm.calc_bounds();
        return bm;
    }

    // Create UV sphere
    static BMesh uv_sphere(float radius = 1.0f, int segments = 32, int rings = 16) {
        BMesh bm;
        bm.verts.reserve((rings + 1) * segments);
        bm.loops.reserve(rings * segments * 3 * 2); // max 6 per quad (triangulated)
        bm.faces.reserve(rings * segments * 2);
        for (int r = 0; r <= rings; ++r) {
            float phi = PI * r / rings;
            float y = cosf(phi);
            float r_sin = sinf(phi);
            for (int s = 0; s < segments; ++s) {
                float theta = 2 * PI * s / segments;
                float x = r_sin * cosf(theta);
                float z = r_sin * sinf(theta);
                BMVert v; v.co = {x*radius, y*radius, z*radius}; bm.verts.push_back(v);
            }
        }
        for (int r = 0; r < rings; ++r) {
            for (int s = 0; s < segments; ++s) {
                int cur_row = r * segments;
                int next_row = (r + 1) * segments;
                int a = cur_row + s;
                int b = cur_row + (s + 1) % segments;
                int c = next_row + (s + 1) % segments;
                int d = next_row + s;
                if (r == 0) {
                    // Top triangle fan
                    BMFace f; f.len = 3;
                    BMLoop* first = nullptr; BMLoop* prev = nullptr;
                    for (int vi : {a, b, c}) {
                        BMLoop l; l.vert = &bm.verts[vi]; l.face = &f; bm.loops.push_back(l);
                        BMLoop* cur = &bm.loops.back();
                        if (prev) { prev->next = cur; cur->prev = prev; }
                        else first = cur;
                        prev = cur;
                    }
                    prev->next = first; first->prev = prev;
                    f.loops = first; bm.faces.push_back(f);
                } else if (r == rings - 1) {
                    // Bottom triangle fan
                    BMFace f; f.len = 3;
                    BMLoop* first = nullptr; BMLoop* prev = nullptr;
                    for (int vi : {a, d, c}) {
                        BMLoop l; l.vert = &bm.verts[vi]; l.face = &f; bm.loops.push_back(l);
                        BMLoop* cur = &bm.loops.back();
                        if (prev) { prev->next = cur; cur->prev = prev; }
                        else first = cur;
                        prev = cur;
                    }
                    prev->next = first; first->prev = prev;
                    f.loops = first; bm.faces.push_back(f);
                } else {
                    // Quad
                    BMFace f; f.len = 4;
                    BMLoop* first = nullptr; BMLoop* prev = nullptr;
                    for (int vi : {a, b, c, d}) {
                        BMLoop l; l.vert = &bm.verts[vi]; l.face = &f; bm.loops.push_back(l);
                        BMLoop* cur = &bm.loops.back();
                        if (prev) { prev->next = cur; cur->prev = prev; }
                        else first = cur;
                        prev = cur;
                    }
                    prev->next = first; first->prev = prev;
                    f.loops = first; bm.faces.push_back(f);
                }
            }
        }
        bm.ensure_lookup_tables();
        bm.calc_normals();
        bm.calc_bounds();
        return bm;
    }
};

} // namespace kronos