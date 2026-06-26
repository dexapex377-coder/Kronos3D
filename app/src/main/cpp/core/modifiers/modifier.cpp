#include "modifier.h"
#include "../mesh/subdiv.h"

namespace kronos {

BMesh SubsurfModifier::apply(const BMesh& mesh) const {
    return subdivide_catmull_clark(mesh, levels_viewport);
}

BMesh MirrorModifier::apply(const BMesh& mesh) const {
    BMesh out = mesh;
    if (use_axis[0]) {
        size_t orig_vert_count = out.verts.size();
        for (size_t i = 0; i < orig_vert_count; ++i) {
            BMVert v = out.verts[i];
            v.co.x = -v.co.x;
            v.no.x = -v.no.x;
            out.verts.push_back(v);
        }
    }
    out.ensure_lookup_tables();
    out.calc_normals();
    out.calc_bounds();
    return out;
}

BMesh BooleanModifier::apply(const BMesh& mesh) const {
    return mesh; // Placeholder
}

BMesh BevelModifier::apply(const BMesh& mesh) const {
    return mesh; // Placeholder
}

BMesh ArrayModifier::apply(const BMesh& mesh) const {
    BMesh out = mesh;
    for (int i = 1; i < count; ++i) {
        size_t base_verts = mesh.verts.size();
        for (size_t v = 0; v < base_verts; ++v) {
            BMVert nv = mesh.verts[v];
            nv.co.x += relative_offset[0] * i + constant_offset[0] * i;
            nv.co.y += relative_offset[1] * i + constant_offset[1] * i;
            nv.co.z += relative_offset[2] * i + constant_offset[2] * i;
            out.verts.push_back(nv);
        }
    }
    out.ensure_lookup_tables();
    out.calc_normals();
    out.calc_bounds();
    return out;
}

} // namespace kronos