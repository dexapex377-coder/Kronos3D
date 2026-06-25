#pragma once

#include "../mesh/bmesh.h"
#include <string>
#include <vector>
#include <memory>

namespace kronos {

struct Modifier;
using ModifierPtr = std::shared_ptr<Modifier>;

enum class ModifierType {
    SUBSURF,
    MIRROR,
    BOOLEAN,
    BEVEL,
    ARRAY,
    DECIMATE,
    REMESH,
    SOLIDIFY,
    DISPLACE,
    ARMATURE,
    HOOK,
    SHRINKWRAP,
    SIMPLE_DEFORM,
    SMOOTH,
    EDGE_SPLIT,
    NORMALS,
    UV_PROJECT,
    VERTEX_WEIGHT,
    MASK,
    TRIANGULATE,
};

struct Modifier {
    ModifierType type;
    std::string name;
    bool enabled = true;
    bool show_viewport = true;
    bool show_render = true;
    bool show_in_editmode = false;
    bool show_on_cage = false;

    Modifier(ModifierType t, const std::string& n) : type(t), name(n) {}
    virtual ~Modifier() = default;

    virtual BMesh apply(const BMesh& mesh) const = 0;
    virtual void ui_draw() {} // ImGui integration
};

struct ModifierStack {
    std::vector<ModifierPtr> modifiers;

    void add(ModifierPtr mod) { modifiers.push_back(std::move(mod)); }
    void remove(size_t index) { if (index < modifiers.size()) modifiers.erase(modifiers.begin() + index); }
    void move_up(size_t index) { if (index > 0) std::swap(modifiers[index], modifiers[index-1]); }
    void move_down(size_t index) { if (index + 1 < modifiers.size()) std::swap(modifiers[index], modifiers[index+1]); }

    BMesh evaluate(const BMesh& base_mesh) const {
        BMesh current = base_mesh;
        for (const auto& mod : modifiers) {
            if (mod->enabled && mod->show_viewport) {
                current = mod->apply(current);
            }
        }
        return current;
    }
};

// Subdivision Surface Modifier
struct SubsurfModifier : Modifier {
    int levels_viewport = 1;
    int levels_render = 2;
    int subdivision_type = 0; // 0=Catmull-Clark, 1=Simple
    bool use_limit_surface = true;
    float quality = 1.0f;
    bool use_uv_smooth = true;

    SubsurfModifier() : Modifier(ModifierType::SUBSURF, "Subdivision Surface") {}

    BMesh apply(const BMesh& mesh) const override {
        // Catmull-Clark subdivision
        return subdivide_catmull_clark(mesh, levels_viewport);
    }
};

// Mirror Modifier
struct MirrorModifier : Modifier {
    bool use_axis[3] = {true, false, false};
    bool use_bisect_axis[3] = {false, false, false};
    bool use_bisect_flip_axis[3] = {false, false, false};
    float tolerance = 0.001f;
    bool use_clip = true;
    bool use_mirror_merge = true;

    MirrorModifier() : Modifier(ModifierType::MIRROR, "Mirror") {}

    BMesh apply(const BMesh& mesh) const override {
        BMesh out = mesh;
        // Mirror along X axis (simplified)
        if (use_axis[0]) {
            size_t orig_vert_count = out.verts.size();
            for (size_t i = 0; i < orig_vert_count; ++i) {
                BMVert v = out.verts[i];
                v.co.x = -v.co.x;
                v.no.x = -v.no.x;
                out.verts.push_back(v);
            }
            // Rebuild faces with mirrored vertices (simplified)
        }
        out.ensure_lookup_tables();
        out.calc_normals();
        out.calc_bounds();
        return out;
    }
};

// Boolean Modifier (using simple CSG - production would use Manifold/CGAL)
struct BooleanModifier : Modifier {
    enum Operation { UNION, INTERSECT, DIFFERENCE };
    Operation operation = UNION;
    BMesh operand; // Target mesh
    bool use_self = false;
    float tolerance = 0.0001f;

    BooleanModifier() : Modifier(ModifierType::BOOLEAN, "Boolean") {}

    BMesh apply(const BMesh& mesh) const override {
        // Placeholder - real implementation needs robust CSG library
        return mesh;
    }
};

// Bevel Modifier
struct BevelModifier : Modifier {
    enum Mode { WEIGHT, VERTEX_GROUP, ANGLE, NONE };
    Mode mode = ANGLE;
    float width = 0.1f;
    int segments = 1;
    float profile = 0.5f;
    bool clamp_overlap = true;
    float angle_limit = 0.523599f; // 30 degrees

    BevelModifier() : Modifier(ModifierType::BEVEL, "Bevel") {}

    BMesh apply(const BMesh& mesh) const override {
        // Placeholder - bevel is complex topology operation
        return mesh;
    }
};

// Array Modifier
struct ArrayModifier : Modifier {
    enum FitType { FIXED, FIT_LENGTH, FIT_CURVE };
    FitType fit_type = FIXED;
    int count = 2;
    float relative_offset[3] = {1.0f, 0.0f, 0.0f};
    float constant_offset[3] = {0.0f, 0.0f, 0.0f};
    bool merge = true;
    float merge_threshold = 0.001f;

    ArrayModifier() : Modifier(ModifierType::ARRAY, "Array") {}

    BMesh apply(const BMesh& mesh) const override {
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
            // Duplicate faces with offset indices (simplified)
        }
        out.ensure_lookup_tables();
        out.calc_normals();
        out.calc_bounds();
        return out;
    }
};

} // namespace kronos