// Argon: tests for the hull contacts, the craft's box against world triangles (logic\hull_contacts.hpp:
// physics_world_contacts 0xd3c70 and what it calls). Run with test.bat / test.sh.
#include "logic/hull_contacts.hpp"
#include "check.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <cstdio>

namespace
{
using argon::test::checks;
using argon::test::failures;

using namespace argon::hull;

// the craft's box, 4 x 2 x 6, in 0x70340's corner order (GameCraft::init)
const std::array<glm::vec3, kBoxCorners> kBody = {glm::vec3(2, -1, 3),
                                                  glm::vec3(-2, -1, 3),
                                                  glm::vec3(2, -1, -3),
                                                  glm::vec3(-2, -1, -3),
                                                  glm::vec3(2, 1, 3),
                                                  glm::vec3(-2, 1, 3),
                                                  glm::vec3(2, 1, -3),
                                                  glm::vec3(-2, 1, -3)};

bool near(const glm::vec3 &a, const glm::vec3 &b)
{
    return glm::length(a - b) < 1e-4f;
}

PlacedBox boxAt(const glm::vec3 &position)
{
    return placeBox(glm::mat3(1.f), position, kBody);
}

// one ground triangle at height y reaching 30 m around the box, its normal down (into the ground) as the tracks' are
std::vector<WorldTriangle> groundAt(float y, uint32_t index = 0)
{
    return {{{glm::vec3(-30, y, -30), glm::vec3(60, y, -30), glm::vec3(-30, y, 60)}, index}};
}

std::vector<WorldTriangle> stackedGround(uint32_t count)
{
    std::vector<WorldTriangle> stack;
    for (uint32_t i = 0; i < count; ++i)
    {
        stack.push_back(groundAt(0.f, i)[0]);
    }
    return stack;
}

std::vector<WorldTriangle> flipped(std::vector<WorldTriangle> triangles)
{
    for (WorldTriangle &t : triangles)
    {
        std::swap(t.corners[1], t.corners[2]);
    }
    return triangles;
}

bool allFrom(const std::vector<Contact> &contacts, NormalSource source)
{
    for (const Contact &c : contacts)
    {
        if (c.source != source)
        {
            return false;
        }
    }
    return true;
}

bool allWithNormal(const std::vector<Contact> &contacts, const glm::vec3 &normal)
{
    for (const Contact &c : contacts)
    {
        if (!near(c.normal, normal))
        {
            return false;
        }
    }
    return true;
}

bool allAtHeight(const std::vector<Contact> &contacts, float y)
{
    for (const Contact &c : contacts)
    {
        if (std::abs(c.point.y - y) > 1e-4f)
        {
            return false;
        }
    }
    return true;
}

bool allBelowIndex(const std::vector<Contact> &contacts, uint32_t index)
{
    for (const Contact &c : contacts)
    {
        if (c.triangle >= index)
        {
            return false;
        }
    }
    return true;
}

glm::vec3 triangleCentre(const PlacedBox &box, int k)
{
    const std::array<int, 3> &c = kBoxTriangleCorners[size_t(k)];
    return (box.corners[size_t(c[0])] + box.corners[size_t(c[1])] + box.corners[size_t(c[2])]) / 3.f;
}

void testBoxTriangleNormalsPointOutOfTheBox()
{
    const PlacedBox box = boxAt(glm::vec3(0));

    int outward = 0;
    for (int k = 0; k < kBoxTriangles; ++k)
    {
        outward += glm::dot(box.normals[size_t(k)], triangleCentre(box, k)) > 0.f ? 1 : 0;
    }

    CHECK(outward == kBoxTriangles);
}

void testEveryBoxFaceHasFourTriangles()
{
    const std::array<glm::vec3, kBoxTriangles> normals = boxTriangleNormals(kBody);

    int down = 0;
    for (const glm::vec3 &n : normals)
    {
        down += near(n, glm::vec3(0, -1, 0)) ? 1 : 0;
    }

    CHECK(down == 4);
}

void testPlacedCornersFollowTheRotation()
{
    const glm::mat3 yaw = glm::mat3(glm::rotate(glm::mat4(1.f), glm::radians(90.f), glm::vec3(0, 1, 0)));

    const PlacedBox box = placeBox(yaw, glm::vec3(10, 0, 0), kBody);

    CHECK(near(box.corners[0], glm::vec3(10 + 3, -1, -2)));
}

void testQueryBoundsComeFromTheTransposedRotation()
{
    const glm::mat3 pitch = glm::mat3(glm::rotate(glm::mat4(1.f), glm::radians(30.f), glm::vec3(1, 0, 0)));
    const glm::mat3 yaw = glm::mat3(glm::rotate(glm::mat4(1.f), glm::radians(60.f), glm::vec3(0, 1, 0)));
    const glm::mat3 rotation = yaw * pitch;

    const Bounds b = queryBounds(rotation, glm::vec3(0), kBody);

    const PlacedBox transposed = placeBox(glm::transpose(rotation), glm::vec3(0), kBody);
    glm::vec3 hi = transposed.corners[0];
    for (const glm::vec3 &p : transposed.corners)
    {
        hi = glm::max(hi, p);
    }
    CHECK(near(b.hi, hi));
}

void testSpansMeetATriangleWithinTheirWidening()
{
    const Bounds query{glm::vec3(-1), glm::vec3(1)};
    const std::array<glm::vec3, 3> beyond = {glm::vec3(1.2f, 0, 0), glm::vec3(3, 0, 0), glm::vec3(3, 0, 1)};

    CHECK(spansMeet(query, beyond));
}

void testSpansMissATriangleHalfAMetreBeyond()
{
    const Bounds query{glm::vec3(-1), glm::vec3(1)};
    const std::array<glm::vec3, 3> beyond = {glm::vec3(1.6f, 0, 0), glm::vec3(3, 0, 0), glm::vec3(3, 0, 1)};

    CHECK(!spansMeet(query, beyond));
}

void testABoxAboveTheGroundHasNoContacts()
{
    const std::vector<Contact> contacts = boxContacts(boxAt(glm::vec3(0, 1.1f, 0)), groundAt(0.f));

    CHECK(contacts.empty());
}

void testABoxSunkIntoTheGroundTouchesIt()
{
    const std::vector<Contact> contacts = boxContacts(boxAt(glm::vec3(0, 0.5f, 0)), groundAt(0.f));

    CHECK(!contacts.empty());
}

void testGroundContactsTakeTheGroundsNormal()
{
    const std::vector<Contact> contacts = boxContacts(boxAt(glm::vec3(0, 0.5f, 0)), groundAt(0.f));

    CHECK(allFrom(contacts, NormalSource::WorldTriangle) && allWithNormal(contacts, glm::vec3(0, -1, 0)));
}

void testGroundContactsLieOnTheGround()
{
    const std::vector<Contact> contacts = boxContacts(boxAt(glm::vec3(0, 0.5f, 0)), groundAt(0.f));

    CHECK(allAtHeight(contacts, 0.f));
}

void testTheNormalFollowsTheTrianglesWinding()
{
    const std::vector<Contact> contacts = boxContacts(boxAt(glm::vec3(0, 0.5f, 0)), flipped(groundAt(0.f)));

    CHECK(!contacts.empty() && allWithNormal(contacts, glm::vec3(0, 1, 0)));
}

void testASpikeThroughTheFloorTakesTheBoxNormal()
{
    const std::vector<WorldTriangle> spike = {{{glm::vec3(-0.2f, -2, 0.3f), glm::vec3(0.2f, -2, 0.3f), glm::vec3(0, -0.5f, 0.3f)}, 0}};

    const std::vector<Contact> contacts = boxContacts(boxAt(glm::vec3(0)), spike);

    CHECK(!contacts.empty() && allFrom(contacts, NormalSource::BoxTriangle) && allWithNormal(contacts, glm::vec3(0, -1, 0)));
}

void testACornerPokingThroughASideCutsEdges()
{
    const std::vector<WorldTriangle> corner = {{{glm::vec3(1, 0, 0), glm::vec3(3, 0, -1), glm::vec3(3, 0, 1)}, 0}};

    const std::vector<Contact> contacts = boxContacts(boxAt(glm::vec3(0)), corner);

    CHECK(!contacts.empty() && allFrom(contacts, NormalSource::CutEdges));
}

void testOnlyTheFirstHundredCandidatesCount()
{
    const std::vector<WorldTriangle> stack = stackedGround(150);

    const std::vector<Contact> contacts = boxContacts(boxAt(glm::vec3(0, 0.5f, 0)), stack);

    CHECK(allBelowIndex(contacts, kMaxCandidates));
}

void testAtMostFiveHundredContacts()
{
    const std::vector<WorldTriangle> stack = stackedGround(50);

    const std::vector<Contact> contacts = boxContacts(boxAt(glm::vec3(0, 0.5f, 0)), stack);

    CHECK(contacts.size() == size_t(kMaxContacts));
}
}  // namespace

int main()
{
    testBoxTriangleNormalsPointOutOfTheBox();
    testEveryBoxFaceHasFourTriangles();
    testPlacedCornersFollowTheRotation();
    testQueryBoundsComeFromTheTransposedRotation();
    testSpansMeetATriangleWithinTheirWidening();
    testSpansMissATriangleHalfAMetreBeyond();
    testABoxAboveTheGroundHasNoContacts();
    testABoxSunkIntoTheGroundTouchesIt();
    testGroundContactsTakeTheGroundsNormal();
    testGroundContactsLieOnTheGround();
    testTheNormalFollowsTheTrianglesWinding();
    testASpikeThroughTheFloorTakesTheBoxNormal();
    testACornerPokingThroughASideCutsEdges();
    testOnlyTheFirstHundredCandidatesCount();
    testAtMostFiveHundredContacts();
    std::printf("%d checks, %d failed\n", checks, failures);
    return failures ? 1 : 0;
}
