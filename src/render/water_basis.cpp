// The water's per-vertex tangent basis (water_basis.hpp): 0xcd170 with 0xccf40.
#include "render/water_basis.hpp"

#include <array>

namespace argon
{

namespace
{

constexpr int kSide = WaterWaves::kVertices;
constexpr float kGridStep = WaterWaves::kGridStep;   // also 0x1a3594
constexpr float kUvPerMetre = 0.0625f;        // 0x1a376c
constexpr float kShortToUnit = WaterWaves::kShortToUnit;

int vertexAt(int i, int j)
{
    return j * kSide + i;
}

glm::vec3 gridPosition(const WaterWaves &waves, int frame, int vertex)
{
    const int i = vertex % kSide, j = vertex / kSide;
    return {kGridStep * float(i), float(waves.height(frame, i, j)) * waves.scale() * kShortToUnit, kGridStep * float(j)};
}

void addTriangle(const WaterWaves &waves, int frame, const std::array<int, 3> &corners, std::vector<BumpBasis> &basis)
{
    std::array<glm::vec3, 3> positions;
    std::array<glm::vec2, 3> uvs;
    for (size_t k = 0; k < 3; ++k)
    {
        positions[k] = gridPosition(waves, frame, corners[k]);
        uvs[k] = glm::vec2(positions[k].x, positions[k].z) * kUvPerMetre;
    }
    const BumpBasis share = triangleTangentShares(positions, uvs);
    for (int corner : corners)
    {
        basis[size_t(corner)].tangent += share.tangent;
        basis[size_t(corner)].binormal += share.binormal;
    }
}

// 0xcdac5's index order
void addTileTriangles(const WaterWaves &waves, int frame, std::vector<BumpBasis> &basis)
{
    for (int j = 0; j + 1 < kSide; ++j)
    {
        for (int i = 0; i + 1 < kSide; ++i)
        {
            const int corner = vertexAt(i, j);
            addTriangle(waves, frame, {corner, corner + 1, corner + kSide}, basis);
            addTriangle(waves, frame, {corner + kSide, corner + 1, corner + kSide + 1}, basis);
        }
    }
}

void shareSums(BumpBasis &first, BumpBasis &second)
{
    first.tangent = second.tangent = first.tangent + second.tangent;
    first.binormal = second.binormal = first.binormal + second.binormal;
}

// 0xcd344: in one pass k = 0..16, the column ends (k, 0) / (k, 16) and then the row ends (0, k) / (16, k), so the
// corners collect their sums in that order
void wrapTileEdges(std::vector<BumpBasis> &basis)
{
    for (int k = 0; k < kSide; ++k)
    {
        shareSums(basis[size_t(vertexAt(k, 0))], basis[size_t(vertexAt(k, kSide - 1))]);
        shareSums(basis[size_t(vertexAt(0, k))], basis[size_t(vertexAt(kSide - 1, k))]);
    }
}

glm::vec3 normalisedOrZero(const glm::vec3 &v)
{
    const float length = glm::length(v);
    return length > 0.f ? v / length : v;
}

void finishVertex(BumpBasis &basis)
{
    basis.tangent = normalisedOrZero(basis.tangent);
    basis.binormal = normalisedOrZero(basis.binormal);
    basis.normal = glm::cross(basis.tangent, basis.binormal);
}

} // namespace

std::vector<BumpBasis> buildWaterBasis(const WaterWaves &waves, int frame)
{
    std::vector<BumpBasis> basis(size_t(kSide) * kSide);
    addTileTriangles(waves, frame, basis);
    wrapTileEdges(basis);
    for (BumpBasis &vertexBasis : basis)
    {
        finishVertex(vertexBasis);
    }
    return basis;
}

} // namespace argon
