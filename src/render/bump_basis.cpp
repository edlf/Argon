// The bump effects' tangent basis (bump_basis.hpp): 0xa2dd0 / 0xa2bb0 and 0xa2a00 / 0xa27e0.
#include "render/bump_basis.hpp"

#include <cmath>
#include <string>

namespace argon
{

namespace
{

constexpr float kSmallestDeterminant = 1e-12f;   // 0x198d94

glm::vec3 positionOf(const Vertex &vertex)
{
    return {vertex.pos[0], vertex.pos[1], vertex.pos[2]};
}

const char *const kDiffuseSpecularBump = "Diffuse Specular Bump Map";
const char *const kSpecAlphaHierarchy = "SpecAlphaHierarchy - 1 Tex";

const std::string &effectName(const Cmm &model, const CmmMesh &mesh)
{
    static const std::string none;
    return mesh.effect >= 0 && size_t(mesh.effect) < model.effects.size() ? model.effects[size_t(mesh.effect)] : none;
}

struct TriangleCorners
{
    uint32_t first, second, third;
};

struct BasisInput
{
    const std::vector<Vertex> &vertices;
    int uvSet;

    glm::vec2 uvOf(uint32_t vertex) const
    {
        return {vertices[vertex].uv[uvSet][0], vertices[vertex].uv[uvSet][1]};
    }
};

void addTriangleShares(const TriangleCorners &corners, const BasisInput &input, std::vector<BumpBasis> &basis)
{
    const std::vector<Vertex> &vertices = input.vertices;
    const BumpBasis share =
        triangleTangentShares({positionOf(vertices[corners.first]), positionOf(vertices[corners.second]), positionOf(vertices[corners.third])},
                              {input.uvOf(corners.first), input.uvOf(corners.second), input.uvOf(corners.third)});
    for (uint32_t corner : {corners.first, corners.second, corners.third})
    {
        basis[corner].tangent += share.tangent;
        basis[corner].binormal += share.binormal;
    }
}

glm::vec3 normalisedOrZero(const glm::vec3 &v)
{
    const float length = glm::length(v);
    return length > 0.f ? v / length : v;
}

void finishVertex(const Vertex &vertex, BumpBasis &basis)
{
    basis.tangent = normalisedOrZero(basis.tangent);
    basis.binormal = normalisedOrZero(basis.binormal);
    basis.normal = glm::cross(basis.tangent, basis.binormal);
    const glm::vec3 vertexNormal = normalisedOrZero(glm::vec3(vertex.nrm[0], vertex.nrm[1], vertex.nrm[2]));
    if (glm::dot(basis.normal, vertexNormal) < 0.f)
    {
        basis.normal = -basis.normal;
    }
}

bool isDegenerate(const TriangleCorners &corners)
{
    return corners.first == corners.second || corners.first == corners.third || corners.second == corners.third;
}

} // namespace

BumpBasis triangleTangentShares(const std::array<glm::vec3, 3> &positions, const std::array<glm::vec2, 3> &uvs)
{
    BumpBasis share;
    for (int axis = 0; axis < 3; ++axis)
    {
        const glm::vec3 edge1(positions[1][axis] - positions[0][axis], uvs[1] - uvs[0]);
        const glm::vec3 edge2(positions[2][axis] - positions[0][axis], uvs[2] - uvs[0]);
        const glm::vec3 cross = glm::cross(edge1, edge2);
        if (std::abs(cross.x) > kSmallestDeterminant)
        {
            share.tangent[axis] = -cross.y / cross.x;
            share.binormal[axis] = -cross.z / cross.x;
        }
    }
    return share;
}

bool usesDiffuseSpecularBump(const Cmm &model, const CmmMesh &mesh)
{
    const std::string &name = effectName(model, mesh);
    return !mesh.skinned && (name == kDiffuseSpecularBump || name == kSpecAlphaHierarchy);
}

int bumpBasisUvSet(const Cmm &model, const CmmMesh &mesh)
{
    return effectName(model, mesh) == kSpecAlphaHierarchy ? 0 : 1;
}

std::vector<BumpBasis> buildBumpBasis(const std::vector<Vertex> &vertices, const std::vector<uint32_t> &triangles, int uvSet)
{
    std::vector<BumpBasis> basis(vertices.size());
    for (size_t i = 0; i + 2 < triangles.size(); i += 3)
    {
        const TriangleCorners corners{triangles[i], triangles[i + 1], triangles[i + 2]};
        if (!isDegenerate(corners))
        {
            addTriangleShares(corners, BasisInput{vertices, uvSet}, basis);
        }
    }
    for (size_t v = 0; v < vertices.size(); ++v)
    {
        finishVertex(vertices[v], basis[v]);
    }
    return basis;
}

void storeBumpBasis(const std::vector<BumpBasis> &basis, std::vector<Vertex> &vertices)
{
    for (size_t i = 0; i < vertices.size() && i < basis.size(); ++i)
    {
        const BumpBasis &vertexBasis = basis[i];
        Vertex &vertex = vertices[i];
        const bool flipped = glm::dot(glm::cross(vertexBasis.tangent, vertexBasis.binormal), vertexBasis.normal) < 0.f;
        vertex.nrm[0] = vertexBasis.tangent.x;
        vertex.nrm[1] = vertexBasis.tangent.y;
        vertex.nrm[2] = vertexBasis.tangent.z;
        vertex.uv[2][0] = vertexBasis.binormal.x;
        vertex.uv[2][1] = vertexBasis.binormal.y;
        vertex.uv[3][0] = vertexBasis.binormal.z;
        vertex.uv[3][1] = flipped ? -1.f : 1.f;
    }
}

} // namespace argon
