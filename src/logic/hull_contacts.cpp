#include "logic/hull_contacts.hpp"

#include <algorithm>
#include <cmath>
#include <optional>

namespace argon::hull
{

const std::array<std::array<int, 3>, kBoxTriangles> kBoxTriangleCorners = {
    {{4, 5, 1}, {1, 0, 4}, {7, 5, 4}, {4, 6, 7}, {3, 7, 6}, {6, 2, 3}, {0, 1, 3}, {3, 2, 0}, {1, 5, 7}, {7, 3, 1}, {6, 4, 0}, {0, 2, 6},
     {5, 1, 0}, {0, 4, 5}, {5, 4, 6}, {6, 7, 5}, {7, 6, 2}, {2, 3, 7}, {1, 3, 2}, {2, 0, 1}, {5, 7, 3}, {3, 1, 5}, {4, 0, 2}, {2, 6, 4}}};

namespace
{

constexpr int kTriangleCorners = 3;
constexpr int kMostCuts = 6;      // every edge of both triangles; anything but 4 is no crossing
constexpr int kCrossingCuts = 4;

// a world triangle that survived so far, with its plane (0xd0c00: +0x18 normal, +0x24 offset) and the box triangles
// reaching in front of that plane (+0x28 count, +0x2c indices)
struct Candidate
{
    WorldTriangle triangle;
    glm::vec3 normal{0.f};
    float offset = 0.f;
    std::vector<int> boxTriangles;
};

// a point where an edge crosses the other triangle's plane, and the edge's index (0: c0-c1, 1: c1-c2, 2: c2-c0)
struct Cut
{
    glm::vec3 point{0.f};
    int edge = 0;
};

struct Cuts
{
    std::array<Cut, kMostCuts> at;
    int count = 0;
};

// one triangle's cut segment sorted along the planes' common line
struct Segment
{
    Cut low;
    Cut high;
    float lo = 0.f;
    float hi = 0.f;
};

// what 0xdb7f0 leaves at +0x20 / +0x24 / +0xc0 / +0x48 / +0x4c
struct Crossing
{
    glm::vec3 from{0.f};
    glm::vec3 to{0.f};
    NormalSource source = NormalSource::WorldTriangle;
    int worldEdge = 0;
    int boxEdge = 0;
};

// the game's normalising: v x (1 / |v|), with no test for a zero vector
glm::vec3 scaledToUnit(const glm::vec3 &v)
{
    return v * (1.f / std::sqrt(glm::dot(v, v)));
}

glm::vec3 triangleNormal(const glm::vec3 &c0, const glm::vec3 &c1, const glm::vec3 &c2)
{
    return scaledToUnit(glm::cross(c0 - c1, c1 - c2));
}

float planeValue(const glm::vec3 &normal, float offset, const glm::vec3 &point)
{
    return glm::dot(normal, point) + offset;
}

std::array<glm::vec3, 3> boxTriangle(const PlacedBox &box, int triangle)
{
    const std::array<int, 3> &corners = kBoxTriangleCorners[size_t(triangle)];
    return {box.corners[size_t(corners[0])], box.corners[size_t(corners[1])], box.corners[size_t(corners[2])]};
}

float boxTriangleOffset(const PlacedBox &box, int triangle)
{
    return -glm::dot(box.normals[size_t(triangle)], box.corners[size_t(kBoxTriangleCorners[size_t(triangle)][0])]);
}

// 0xd0c00: the first 100 candidates with their planes
std::vector<Candidate> planesOf(const std::vector<WorldTriangle> &found)
{
    std::vector<Candidate> list;
    const size_t kept = std::min(found.size(), size_t(kMaxCandidates));
    for (size_t i = 0; i < kept; ++i)
    {
        Candidate candidate;
        candidate.triangle = found[i];
        const std::array<glm::vec3, 3> &corners = found[i].corners;
        candidate.normal = triangleNormal(corners[0], corners[1], corners[2]);
        candidate.offset = -glm::dot(candidate.normal, corners[0]);
        list.push_back(candidate);
    }
    return list;
}

// the lists drop an entry by moving the last one into its place (0xd0d80, 0xd0fa9)
void swapRemove(std::vector<Candidate> &list, size_t index)
{
    list[index] = list.back();
    list.pop_back();
}

bool allInFront(const glm::vec3 &normal, float offset, const std::array<glm::vec3, 3> &corners)
{
    return planeValue(normal, offset, corners[0]) > 0.f && planeValue(normal, offset, corners[1]) > 0.f && planeValue(normal, offset, corners[2]) > 0.f;
}

bool anyInFront(const glm::vec3 &normal, float offset, const std::array<glm::vec3, 3> &corners)
{
    return planeValue(normal, offset, corners[0]) > 0.f || planeValue(normal, offset, corners[1]) > 0.f || planeValue(normal, offset, corners[2]) > 0.f;
}

// 0xd0d80 for each box triangle in turn: a world triangle wholly in front of the box triangle's plane is dropped
void dropInFrontOfBoxTriangles(const PlacedBox &box, std::vector<Candidate> &list)
{
    for (int k = 0; k < kBoxTriangles; ++k)
    {
        const float offset = boxTriangleOffset(box, k);
        size_t next = 0;
        while (next < list.size())
        {
            if (allInFront(box.normals[size_t(k)], offset, list[next].triangle.corners))
            {
                swapRemove(list, next);
                continue;
            }
            ++next;
        }
    }
}

// 0xd0e50: a box triangle with a corner in front of the world triangle's plane is kept, and once one is, so is every
// later one; a world triangle keeping none is dropped
void keepBoxTrianglesReachingIn(const PlacedBox &box, std::vector<Candidate> &list)
{
    size_t next = 0;
    while (next < list.size())
    {
        Candidate &candidate = list[next];
        bool reached = false;
        for (int k = 0; k < kBoxTriangles; ++k)
        {
            reached = reached || anyInFront(candidate.normal, candidate.offset, boxTriangle(box, k));
            if (reached)
            {
                candidate.boxTriangles.push_back(k);
            }
        }
        if (!reached)
        {
            swapRemove(list, next);
            continue;
        }
        ++next;
    }
}

// a plane: the points p with normal . p + offset = 0
struct Plane
{
    glm::vec3 normal;
    float offset;
};

// a triangle's edge e, from corner e to corner e + 1
struct Edge
{
    glm::vec3 p, q;
    int index;
};

// 0xdb7f0's edge test: values are n . p without the plane offset; t = -(vq + offset) / (vp - vq), kept on 0..1
void cutEdge(const Edge &edge, const Plane &plane, Cuts &cuts)
{
    const glm::vec3 &p = edge.p, &q = edge.q;
    const float vp = glm::dot(plane.normal, p), vq = glm::dot(plane.normal, q), offset = plane.offset;
    if (vp == vq)
    {
        return;
    }
    const float crossing = -((vq + offset) / (vp - vq));
    if (!(crossing >= 0.f && crossing <= 1.f))
    {
        return;
    }
    cuts.at[size_t(cuts.count)] = {crossing * p + (1.f - crossing) * q, edge.index};
    ++cuts.count;
}

void cutTriangle(const std::array<glm::vec3, 3> &corners, const Plane &plane, Cuts &cuts)
{
    for (int e = 0; e < kTriangleCorners; ++e)
    {
        cutEdge(Edge{corners[size_t(e)], corners[size_t((e + 1) % kTriangleCorners)], e}, plane, cuts);
    }
}

Segment sortedAlong(const glm::vec3 &line, const Cut &first, const Cut &second)
{
    const float firstAlong = glm::dot(line, first.point);
    const float secondAlong = glm::dot(line, second.point);
    if (firstAlong <= secondAlong)
    {
        return {first, second, firstAlong, secondAlong};
    }
    return {second, first, secondAlong, firstAlong};
}

// 0xdbf55..0xdc2d0: the world segment w (cuts 0, 1) against the box segment b (cuts 2, 3)
std::optional<Crossing> overlapOf(const Segment &worldSegment, const Segment &boxSegment)
{
    if ((boxSegment.lo < worldSegment.lo && boxSegment.hi < worldSegment.lo) || (worldSegment.hi < boxSegment.lo && worldSegment.hi < boxSegment.hi))
    {
        return std::nullopt;
    }
    if (worldSegment.lo < boxSegment.lo && boxSegment.hi < worldSegment.hi)
    {
        return Crossing{boxSegment.low.point, boxSegment.high.point, NormalSource::WorldTriangle, 0, 0};
    }
    if (boxSegment.lo < worldSegment.lo)
    {
        if (worldSegment.hi < boxSegment.hi)
        {
            return Crossing{worldSegment.low.point, worldSegment.high.point, NormalSource::BoxTriangle, 0, 0};
        }
        return Crossing{worldSegment.low.point, boxSegment.high.point, NormalSource::CutEdges, worldSegment.low.edge, boxSegment.high.edge};
    }
    // 0xdc298: here the world edge slot gets the box cut's edge and the box edge slot the world cut's, as shipped
    return Crossing{boxSegment.low.point, worldSegment.high.point, NormalSource::CutEdges, boxSegment.low.edge, worldSegment.high.edge};
}

// 0xdb7f0 with the world triangle as its first triangle (normal and offset given) and the box triangle as its second
std::optional<Crossing> crossTriangles(const Candidate &world, const PlacedBox &box, int triangle)
{
    const std::array<glm::vec3, 3> boxCorners = boxTriangle(box, triangle);
    const glm::vec3 &boxNormal = box.normals[size_t(triangle)];
    Cuts cuts;
    cutTriangle(world.triangle.corners, Plane{boxNormal, -glm::dot(boxNormal, boxCorners[0])}, cuts);
    cutTriangle(boxCorners, Plane{world.normal, world.offset}, cuts);
    if (cuts.count != kCrossingCuts)
    {
        return std::nullopt;
    }
    const glm::vec3 worldSpan = cuts.at[1].point - cuts.at[0].point;
    const glm::vec3 boxSpan = cuts.at[3].point - cuts.at[2].point;
    const glm::vec3 line = scaledToUnit(glm::length(boxSpan) >= glm::length(worldSpan) ? boxSpan : worldSpan);
    return overlapOf(sortedAlong(line, cuts.at[0], cuts.at[1]), sortedAlong(line, cuts.at[2], cuts.at[3]));
}

glm::vec3 edgeVector(const std::array<glm::vec3, 3> &corners, int edge)
{
    return corners[size_t((edge + 1) % kTriangleCorners)] - corners[size_t(edge)];
}

// 0xdc2e0: cross(world edge, box edge), normalised unless it is zero
glm::vec3 cutEdgesNormal(const Candidate &world, const PlacedBox &box, int triangle, const Crossing &crossing)
{
    const glm::vec3 normal = glm::cross(edgeVector(world.triangle.corners, crossing.worldEdge), edgeVector(boxTriangle(box, triangle), crossing.boxEdge));
    if (normal == glm::vec3(0.f))
    {
        return normal;
    }
    return scaledToUnit(normal);
}

glm::vec3 contactNormal(const Candidate &world, const PlacedBox &box, int corner, const Crossing &crossing)
{
    if (crossing.source == NormalSource::BoxTriangle)
    {
        return scaledToUnit(box.normals[size_t(corner)]);
    }
    if (crossing.source == NormalSource::CutEdges)
    {
        return cutEdgesNormal(world, box, corner, crossing);
    }
    return scaledToUnit(world.normal);
}

long spanStep(float v)
{
    return std::lrint(v * kSpanSteps);
}

// 0xdd2d0: -1 - round(v x -4) for a span's start, 1 - round(v x -4) for its end
long spanStartStep(float v)
{
    return -1 - std::lrint(v * -kSpanSteps);
}

long spanEndStep(float v)
{
    return 1 - std::lrint(v * -kSpanSteps);
}

// 0xd4108..0xd43a6
Contact contactOf(const Candidate &world, const PlacedBox &box, int corner, const Crossing &crossing)
{
    Contact contact;
    contact.triangle = world.triangle.index;
    contact.source = crossing.source;
    contact.point = (crossing.from + crossing.to) * 0.5f;
    contact.normal = contactNormal(world, box, corner, crossing);
    const glm::vec3 span = crossing.from - crossing.to;
    contact.squaredSpan = glm::dot(span, span);
    return contact;
}

}  // namespace

std::array<glm::vec3, kBoxTriangles> boxTriangleNormals(const std::array<glm::vec3, kBoxCorners> &bodyCorners)
{
    std::array<glm::vec3, kBoxTriangles> normals;
    for (int k = 0; k < kBoxTriangles; ++k)
    {
        const std::array<int, 3> &corners = kBoxTriangleCorners[size_t(k)];
        normals[size_t(k)] = triangleNormal(bodyCorners[size_t(corners[0])], bodyCorners[size_t(corners[1])], bodyCorners[size_t(corners[2])]);
    }
    return normals;
}

PlacedBox placeBox(const glm::mat3 &rotation, const glm::vec3 &position, const std::array<glm::vec3, kBoxCorners> &bodyCorners)
{
    PlacedBox box;
    for (int i = 0; i < kBoxCorners; ++i)
    {
        box.corners[size_t(i)] = rotation * bodyCorners[size_t(i)] + position;
    }
    const std::array<glm::vec3, kBoxTriangles> bodyNormals = boxTriangleNormals(bodyCorners);
    for (int k = 0; k < kBoxTriangles; ++k)
    {
        box.normals[size_t(k)] = rotation * bodyNormals[size_t(k)];
    }
    return box;
}

Bounds queryBounds(const glm::mat3 &rotation, const glm::vec3 &position, const std::array<glm::vec3, kBoxCorners> &bodyCorners)
{
    const glm::mat3 transposed = glm::transpose(rotation);
    Bounds bounds;
    bounds.lo = bounds.hi = transposed * bodyCorners[0] + position;
    for (int i = 1; i < kBoxCorners; ++i)
    {
        const glm::vec3 corner = transposed * bodyCorners[size_t(i)] + position;
        bounds.lo = glm::min(bounds.lo, corner);
        bounds.hi = glm::max(bounds.hi, corner);
    }
    return bounds;
}

bool spansMeet(const Bounds &query, const std::array<glm::vec3, 3> &corners)
{
    const glm::vec3 lo = glm::min(glm::min(corners[0], corners[1]), corners[2]);
    const glm::vec3 hi = glm::max(glm::max(corners[0], corners[1]), corners[2]);
    for (int axis = 0; axis < 3; ++axis)
    {
        if (spanStartStep(lo[axis]) > spanStep(query.hi[axis]) || spanEndStep(hi[axis]) < spanStep(query.lo[axis]))
        {
            return false;
        }
    }
    return true;
}

std::vector<Contact> boxContacts(const PlacedBox &box, const std::vector<WorldTriangle> &candidates)
{
    std::vector<Candidate> list = planesOf(candidates);
    dropInFrontOfBoxTriangles(box, list);
    keepBoxTrianglesReachingIn(box, list);
    std::vector<Contact> contacts;
    for (const Candidate &world : list)
    {
        for (int triangle : world.boxTriangles)
        {
            const std::optional<Crossing> crossing = crossTriangles(world, box, triangle);
            if (!crossing)
            {
                continue;
            }
            if (contacts.size() == size_t(kMaxContacts))
            {
                return contacts;
            }
            contacts.push_back(contactOf(world, box, triangle, *crossing));
        }
    }
    return contacts;
}

}  // namespace argon::hull
