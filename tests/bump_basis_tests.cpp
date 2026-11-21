// Argon: tests for the bump effects' tangent basis (render/bump_basis.hpp, 0xa2dd0 / 0xa2bb0, 0xa2a00 / 0xa27e0).
// Run with test.bat / test.sh.
#include "render/bump_basis.hpp"
#include "check.hpp"

#include <cmath>
#include <cstdio>

using namespace argon;

namespace
{
using argon::test::checks;
using argon::test::failures;

bool near(const glm::vec3 &a, const glm::vec3 &b)
{
    return glm::length(a - b) < 1e-5f;
}

// a vertex on the ground (y = 0, normal up) whose normal-map uv (set 1) is (x / uScale, z)
Vertex groundVertex(float x, float z, float uScale)
{
    Vertex v{};
    v.pos[0] = x;
    v.pos[2] = z;
    v.nrm[1] = 1.f;
    v.uv[1][0] = x / uScale;
    v.uv[1][1] = z;
    return v;
}

std::vector<Vertex> groundQuad(float uScale)
{
    return {groundVertex(0, 0, uScale), groundVertex(1, 0, uScale), groundVertex(1, 1, uScale), groundVertex(0, 1, uScale)};
}

const std::vector<uint32_t> kQuadTriangles = {0, 1, 2, 0, 2, 3};

void tangentFollowsTheNormalMapsU()
{
    const std::vector<BumpBasis> basis = buildBumpBasis(groundQuad(1.f), kQuadTriangles);
    CHECK(near(basis[0].tangent, {1, 0, 0}));
}

void binormalFollowsTheNormalMapsV()
{
    const std::vector<BumpBasis> basis = buildBumpBasis(groundQuad(1.f), kQuadTriangles);
    CHECK(near(basis[2].binormal, {0, 0, 1}));
}

void normalIsTurnedToTheVertexNormal()
{   // x cross z points down; the vertex normal points up
    const std::vector<BumpBasis> basis = buildBumpBasis(groundQuad(1.f), kQuadTriangles);
    CHECK(near(basis[1].normal, {0, 1, 0}));
}

void mirroredUvTurnsTheTangent()
{
    const std::vector<BumpBasis> basis = buildBumpBasis(groundQuad(-2.f), kQuadTriangles);
    CHECK(near(basis[3].tangent, {-1, 0, 0}));
}

void windingDoesNotMatter()
{
    const std::vector<BumpBasis> forward = buildBumpBasis(groundQuad(1.f), kQuadTriangles);
    const std::vector<BumpBasis> backward = buildBumpBasis(groundQuad(1.f), {0, 2, 1, 0, 3, 2});
    CHECK(near(forward[0].tangent, backward[0].tangent) && near(forward[0].binormal, backward[0].binormal));
}

void degenerateTrianglesAddNothing()
{
    const std::vector<BumpBasis> basis = buildBumpBasis(groundQuad(1.f), {0, 0, 1});
    CHECK(near(basis[0].tangent, {0, 0, 0}));
}

void storedBasisRebuildsTheNormal()
{   // what mesh.vert does: normal = side x (tangent x binormal)
    std::vector<Vertex> quad = groundQuad(1.f);
    const std::vector<BumpBasis> basis = buildBumpBasis(quad, kQuadTriangles);
    storeBumpBasis(basis, quad);
    const glm::vec3 tangent(quad[0].nrm[0], quad[0].nrm[1], quad[0].nrm[2]);
    const glm::vec3 binormal(quad[0].uv[2][0], quad[0].uv[2][1], quad[0].uv[3][0]);
    CHECK(near(quad[0].uv[3][1] * glm::cross(tangent, binormal), basis[0].normal));
}

// the quad with its colour uv (set 0) turned a quarter: u along z
std::vector<Vertex> quadWithTurnedColourUv()
{
    std::vector<Vertex> quad = groundQuad(1.f);
    for (Vertex &v : quad)
    {
        v.uv[0][0] = v.pos[2];
        v.uv[0][1] = v.pos[0];
    }
    return quad;
}

void specAlphaHierarchyBuildsOverTheColourUv()
{   // 0xa2a00 / 0xa27e0 read UV set 0
    const std::vector<BumpBasis> basis = buildBumpBasis(quadWithTurnedColourUv(), kQuadTriangles, 0);
    CHECK(near(basis[0].tangent, {0, 0, 1}));
}

Cmm modelWithEffect(const char *name)
{
    Cmm model;
    model.effects = {name};
    return model;
}

CmmMesh meshWithEffect()
{
    CmmMesh mesh;
    mesh.effect = 0;
    return mesh;
}

void specAlphaHierarchyDrawsAsTheBumpEffect()
{
    const Cmm model = modelWithEffect("SpecAlphaHierarchy - 1 Tex");
    CHECK(usesDiffuseSpecularBump(model, meshWithEffect()) && bumpBasisUvSet(model, meshWithEffect()) == 0);
}

void diffuseSpecularBumpBuildsOverTheNormalMapUv()
{
    const Cmm model = modelWithEffect("Diffuse Specular Bump Map");
    CHECK(usesDiffuseSpecularBump(model, meshWithEffect()) && bumpBasisUvSet(model, meshWithEffect()) == 1);
}

void otherEffectsAreNoBump()
{
    CHECK(!usesDiffuseSpecularBump(modelWithEffect("Fast Hierarchy - 1 Texture"), meshWithEffect()));
}

} // namespace

int main()
{
    tangentFollowsTheNormalMapsU();
    binormalFollowsTheNormalMapsV();
    normalIsTurnedToTheVertexNormal();
    mirroredUvTurnsTheTangent();
    windingDoesNotMatter();
    degenerateTrianglesAddNothing();
    storedBasisRebuildsTheNormal();
    specAlphaHierarchyBuildsOverTheColourUv();
    specAlphaHierarchyDrawsAsTheBumpEffect();
    diffuseSpecularBumpBuildsOverTheNormalMapUv();
    otherEffectsAreNoBump();
    std::printf("bump_basis_tests: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
