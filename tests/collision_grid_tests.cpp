// Argon: tests for the collision grid's queries (craft\drive.hpp: CollisionGrid::trianglesInBox, which the hull
// contacts and the ship shadows read; rayDown, segment and rayHitsTriangle). Run with test.bat / test.sh.
#include "check.hpp"
#include "craft/drive.hpp"

#include <algorithm>
#include <cstdio>

using namespace argon;

namespace
{
using argon::test::checks;
using argon::test::failures;

const float kCell = 10.f;

// a fixed scatter of triangles, small ones inside a cell and long ones over many cells
std::vector<glm::vec3> scatteredTriangles()
{
    std::vector<glm::vec3> tris;
    uint32_t seed = 12345;
    for (int t = 0; t < 300; ++t)
    {
        seed = seed * 214013u + 2531011u;
        const float x = float(seed % 1000) / 10.f, z = float((seed >> 10) % 1000) / 10.f;
        const float span = t % 5 == 0 ? 35.f : 3.f;
        tris.push_back({x, 0.f, z});
        tris.push_back({x + span, 1.f, z});
        tris.push_back({x, 2.f, z + span});
    }
    return tris;
}

std::vector<uint32_t> bruteForceInBox(const std::vector<glm::vec3> &tris, const glm::vec3 &lo, const glm::vec3 &hi)
{
    std::vector<uint32_t> found;
    for (uint32_t t = 0; t < tris.size() / 3; ++t)
    {
        const glm::vec3 mn = glm::min(glm::min(tris[t * 3], tris[t * 3 + 1]), tris[t * 3 + 2]);
        const glm::vec3 mx = glm::max(glm::max(tris[t * 3], tris[t * 3 + 1]), tris[t * 3 + 2]);
        if (!(mx.x < lo.x || mn.x > hi.x || mx.y < lo.y || mn.y > hi.y || mx.z < lo.z || mn.z > hi.z))
        {
            found.push_back(t);
        }
    }
    return found;
}

std::vector<uint32_t> sorted(std::vector<uint32_t> list)
{
    std::sort(list.begin(), list.end());
    return list;
}

void boxQueryFindsEveryOverlappingTriangleOnce()
{
    const std::vector<glm::vec3> tris = scatteredTriangles();
    CollisionGrid grid;
    grid.build(tris, kCell);
    const glm::vec3 boxes[][2] = {{{20, -1, 20}, {45, 5, 38}}, {{0, -1, 0}, {140, 5, 140}}, {{63, -1, 7}, {64, 5, 8}}};
    for (const auto &box : boxes)
    {
        std::vector<uint32_t> found;
        grid.trianglesInBox(box[0], box[1], found);
        CHECK(sorted(found) == bruteForceInBox(tris, box[0], box[1]));
    }
}

void boxOutsideTheHeightFindsNothing()
{
    CollisionGrid grid;
    grid.build(scatteredTriangles(), kCell);
    std::vector<uint32_t> found{7};
    grid.trianglesInBox({0, 10, 0}, {140, 20, 140}, found);
    CHECK(found.empty());
}

// one flat triangle at height 5 over x 0..20, z 0..20, wound so its own normal points down
std::vector<glm::vec3> floorAtFive()
{
    return {{0.f, 5.f, 0.f}, {20.f, 5.f, 0.f}, {0.f, 5.f, 20.f}};
}

void rayDownFindsTheFloorBelow()
{
    CollisionGrid grid;
    grid.build(floorAtFive(), kCell);

    GroundHit hit;
    const bool found = grid.rayDown({4.f, 9.f, 4.f}, 10.f, hit);

    CHECK(found && hit.y == 5.f);
}

void rayDownTurnsTheNormalUp()
{
    CollisionGrid grid;
    grid.build(floorAtFive(), kCell);

    GroundHit hit;
    const bool found = grid.rayDown({4.f, 9.f, 4.f}, 10.f, hit);

    CHECK(found && hit.normal.y > 0.99f);
}

void rayDownStopsAtTheMaxDrop()
{
    CollisionGrid grid;
    grid.build(floorAtFive(), kCell);

    GroundHit hit;
    const bool found = grid.rayDown({4.f, 9.f, 4.f}, 3.f, hit);

    CHECK(!found);
}

void rayDownMissesBesideTheGrid()
{
    CollisionGrid grid;
    grid.build(floorAtFive(), kCell);

    GroundHit hit;
    const bool found = grid.rayDown({40.f, 9.f, 4.f}, 10.f, hit);

    CHECK(!found);
}

void segmentGivesHowFarAlong()
{
    CollisionGrid grid;
    grid.build(floorAtFive(), kCell);

    SegmentHit hit;
    const bool found = grid.segment({4.f, 9.f, 4.f}, {4.f, 1.f, 4.f}, hit);

    CHECK(found && hit.along == 0.5f && hit.triangle == 0);
}

void segmentShortOfTheFloorMisses()
{
    CollisionGrid grid;
    grid.build(floorAtFive(), kCell);

    SegmentHit hit;
    const bool found = grid.segment({4.f, 9.f, 4.f}, {4.f, 6.f, 4.f}, hit);

    CHECK(!found);
}

void rayPointingAwayMisses()
{
    const std::vector<glm::vec3> floor = floorAtFive();

    float distance;
    const bool found = rayHitsTriangle(Ray{{4.f, 9.f, 4.f}, {0.f, 1.f, 0.f}}, floor.data(), distance);

    CHECK(!found);
}

}  // namespace

int main()
{
    boxQueryFindsEveryOverlappingTriangleOnce();
    boxOutsideTheHeightFindsNothing();
    rayDownFindsTheFloorBelow();
    rayDownTurnsTheNormalUp();
    rayDownStopsAtTheMaxDrop();
    rayDownMissesBesideTheGrid();
    segmentGivesHowFarAlong();
    segmentShortOfTheFloorMisses();
    rayPointingAwayMisses();
    std::printf("collision_grid_tests: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
