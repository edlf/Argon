// Argon: what the crafts fly over: the collision, its spans, the water (GameCraft, craft\craftsim.hpp; docs\physics\README.md).
#include "craft/craftsim.hpp"
#include "files/game_path.hpp"
#include "logic/blast_push.hpp"
#include "logic/rumble_rules.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>

#include <glm/gtc/type_ptr.hpp>

namespace argon
{

// ------------------------------------------------------------------ world
bool SimWorld::segment(const glm::vec3 &from, const glm::vec3 &to, glm::vec3 &hit, glm::vec3 &normal, TriangleSurface &surface) const
{
    SegmentHit found;
    if (!grid || !grid->segment(from, to, found))
    {
        return false;
    }
    normal = found.normal;
    hit = from + (to - from) * found.along;
    surface = surfaceOf(uint32_t(found.triangle));
    return true;
}

std::vector<hull::WorldTriangle> SimWorld::spanTriangles(const hull::Bounds &query) const
{
    std::vector<hull::WorldTriangle> found;
    if (!grid)
    {
        return found;
    }
    const glm::vec3 widening(2.f / hull::kSpanSteps);   // the spans' widening and rounding, so the grid misses none
    std::vector<uint32_t> near;
    grid->trianglesInBox(query.lo - widening, query.hi + widening, near);
    std::sort(near.begin(), near.end());
    for (uint32_t triangle : near)
    {
        const glm::vec3 *corner = grid->triangle(triangle);
        const std::array<glm::vec3, 3> corners{corner[0], corner[1], corner[2]};
        if (hull::spansMeet(query, corners))
        {
            found.push_back({corners, triangle});
        }
    }
    return found;
}

TriangleSurface SimWorld::surfaceOf(uint32_t triangle) const
{
    return (surf && triangle < surf->size()) ? (*surf)[triangle] : TriangleSurface{};
}

bool SimWorld::waterDepth(const glm::vec3 &point, float &depth) const
{
    return waterDepthAt(water, waves, point, clock, depth);
}

} // namespace argon
