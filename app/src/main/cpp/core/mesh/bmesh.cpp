#include "bmesh.h"
#include <cmath>

namespace kronos {

BMesh BMesh::cube(float size) {
    BMesh bm;
    float h = size * 0.5f;
    Vec3 coords[8] = {
        {-h,-h,-h}, {h,-h,-h}, {h,h,-h}, {-h,h,-h},
        {-h,-h,h},  {h,-h,h},  {h,h,h},  {-h,h,h}
    };
    for (int i = 0; i < 8; ++i) {
        BMVert v; v.co = coords[i]; bm.verts.push_back(v);
    }
    int face_verts[6][4] = {
        {0,1,2,3}, {4,7,6,5}, {0,4,5,1},
        {1,5,6,2}, {2,6,7,3}, {3,7,4,0}
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

BMesh BMesh::uv_sphere(float radius, int segments, int rings) {
    BMesh bm;
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

} // namespace kronos