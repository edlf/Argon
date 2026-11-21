// Argon: the game's hull contacts, the craft's 4 x 2 x 6 box against the track's triangles (physics_world_contacts
// 0xd3c70, run by physics_substep 0xcffb0 after the box points 0xd1620), without the craft around it
// (docs\physics\README.md "Integration"). GameCraft::collide feeds the contacts to the response 0xd5140;
// tests\hull_contacts_tests.cpp.
//
// The box is 24 triangles (0x6fdc0: every face split along both diagonals). A world triangle near the box is a
// candidate (the span query 0xd3770, at most 100); a box triangle plane with the whole triangle in front of it drops it
// (0xd0d80); the box triangles reaching in front of the triangle's own plane are kept for it (0xd0e50); each kept pair
// is crossed (0xdb7f0): both triangles cut by the other's plane give two segments on the planes' common line, and how
// the segments overlap gives the contact (at most 500): its point is the middle of the overlap's ends, its normal the
// world triangle's (the box triangle's segment inside the world triangle's), the box triangle's (the other way round) or
// the two cut edges' cross product (0xdc2e0, the segments only partly overlapping).
#pragma once
#include <glm/glm.hpp>

#include <array>
#include <cstdint>
#include <vector>

namespace argon::hull
{

constexpr int kBoxCorners = 8;
constexpr int kBoxTriangles = 24;
constexpr int kMaxCandidates = 100;   // 0xd0c00
constexpr int kMaxContacts = 500;     // 0xd439e
constexpr float kSpanSteps = 4.f;     // 0xdd4a0 / 0xdd5c0: the span lists keep bounds in 0.25 m steps (0x1a3444)

// 0x6fdc0 (fastcall 0xd14b0 x 24): each triangle's corners (the craft's corner order, GameCraft::init)
extern const std::array<std::array<int, 3>, kBoxTriangles> kBoxTriangleCorners;

// the box in the world for one state block (0xd1620): corners R v + x (+0x38), triangle normals R n (+0x54)
struct PlacedBox
{
    std::array<glm::vec3, kBoxCorners> corners;
    std::array<glm::vec3, kBoxTriangles> normals;
};

struct Bounds
{
    glm::vec3 lo{0.f};
    glm::vec3 hi{0.f};
};

// a world triangle near the box: its corners in the order 0xd3c70 reads them, and the caller's index for it
struct WorldTriangle
{
    std::array<glm::vec3, 3> corners;
    uint32_t index = 0;
};

// where a contact's normal comes from (contact +0x10)
enum class NormalSource
{
    BoxTriangle = 0,
    CutEdges = 1,
    WorldTriangle = 2   // the only one the response 0xd5140 acts on (physics_substep pushes 0 as its second argument)
};

// one contact record (0x34 bytes at collision +0x1c0c): +0x14 point, +0x20 normal, +0x2c the squared length between
// the two segment ends the point is the middle of
struct Contact
{
    uint32_t triangle = 0;
    NormalSource source = NormalSource::WorldTriangle;
    glm::vec3 point{0.f};
    glm::vec3 normal{0.f};
    float squaredSpan = 0.f;
};

// 0x6fdc0 / 0xd14b0: normalise(cross(c0 - c1, c1 - c2)) of each box triangle, in the body frame
std::array<glm::vec3, kBoxTriangles> boxTriangleNormals(const std::array<glm::vec3, kBoxCorners> &bodyCorners);

// 0xd1620: rotation (columns right, up, forward) and position place the body corners and normals
PlacedBox placeBox(const glm::mat3 &rotation, const glm::vec3 &position, const std::array<glm::vec3, kBoxCorners> &bodyCorners);

// 0xd1620's bounds (+0x6c..+0x80) come from its other corner set (+0x18), placed by the transposed rotation
// (physics_substep 0xcffb0 passes R^T): the span query looks around R^T v + x, not around the box itself
Bounds queryBounds(const glm::mat3 &rotation, const glm::vec3 &position, const std::array<glm::vec3, kBoxCorners> &bodyCorners);

// 0xd3770: a triangle's spans hold its bounds rounded to 0.25 m and widened by one step (0xdd2d0); it is found when
// they meet the rounded query on every axis (ties at a step depend on the span cursor's history, 0xdd4a0, and count)
bool spansMeet(const Bounds &query, const std::array<glm::vec3, 3> &corners);

// 0xd3c70: the contacts of the box with the candidates, in span order (only the first kMaxCandidates are used)
std::vector<Contact> boxContacts(const PlacedBox &box, const std::vector<WorldTriangle> &candidates);

}  // namespace argon::hull
