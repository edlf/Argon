// The per-vertex tangent basis of the "Diffuse Specular Bump Map" and "SpecAlphaHierarchy - 1 Tex" effects
// (docs/models/effects.md "Effects"). Their prepares (0x7d5c0 / 0x7ca40) copy each mesh into a 0x50-byte vertex and build the
// basis with 0xa2dd0 / 0xa2a00: for every triangle (strip degenerates skipped) and every axis c, the edges (dc, ds, dt)
// from vertex 0 to 1 and 2 over a UV set (Diffuse Specular Bump Map: set 1, the normal map's, 0xa2bb0;
// SpecAlphaHierarchy: set 0, the colour's, 0xa27e0) give the cross product (A, B, C); when |A| > 1e-12 the triangle
// adds -B / A to the tangent's c and -C / A to the binormal's c of its three vertices. Then per vertex: tangent and
// binormal normalised, normal = tangent x binormal (not normalised), negated when it points away from the vertex normal.
#pragma once
#include "model/cmm.hpp"

#include <array>
#include <cstdint>
#include <vector>
#include <glm/glm.hpp>

namespace argon
{

struct BumpBasis
{
    glm::vec3 tangent{0.f};    // vertex shader v3: along the normal map's u
    glm::vec3 binormal{0.f};   // v4: along its v
    glm::vec3 normal{0.f};     // v5: tangent x binormal, facing the vertex normal
};

// 0xa2bb0 (and the water's 0xccf40): one triangle's share of its corners' tangent and binormal (normal unused)
BumpBasis triangleTangentShares(const std::array<glm::vec3, 3> &positions, const std::array<glm::vec2, 3> &uvs);

// the mesh's material selects one of the two effects (material +0xe0 -> the model's effect record named so); both
// draw alike (0x7d6e0 / 0x7d0e0); skinned meshes draw with their own vertex shaders (0xa3df0)
bool usesDiffuseSpecularBump(const Cmm &model, const CmmMesh &mesh);

// the UV set its basis is built over: 1 for Diffuse Specular Bump Map, 0 for SpecAlphaHierarchy
int bumpBasisUvSet(const Cmm &model, const CmmMesh &mesh);

// triangles: a triangle list of indices into vertices
std::vector<BumpBasis> buildBumpBasis(const std::vector<Vertex> &vertices, const std::vector<uint32_t> &triangles, int uvSet = 1);

// The viewer's vertex has no tangent slots; these meshes (vertex type 1) leave UV sets 2 / 3 as copies of set 1, so the
// basis travels there: normal = tangent, uv[2] = binormal x y, uv[3] = binormal z and the normal's side (+-1).
// mesh.vert rebuilds the normal as side x (tangent x binormal).
void storeBumpBasis(const std::vector<BumpBasis> &basis, std::vector<Vertex> &vertices);

} // namespace argon
