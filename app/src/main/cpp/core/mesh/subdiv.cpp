#include "subdiv.h"

namespace kronos {

BMesh subdivide_catmull_clark(const BMesh& bm, int level) {
    BMesh result = bm;
    for (int i = 0; i < level; ++i) {
        result.calc_normals();
    }
    return result;
}

} // namespace kronos