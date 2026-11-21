// Argon: reading a model folder off the render thread (model_source.hpp).
#include "model/model_source.hpp"
#include "files/game_path.hpp"
#include "original/cmm_records.hpp"
#include "render/bump_basis.hpp"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <glm/gtc/type_ptr.hpp>

namespace
{

// tex%u.dds, or the low-detail set (instance +0x5e8: data\%s\lotex%d.dds) when there is no tex%u (frontend\earth)
std::string texturePath(const std::string &folder, uint32_t index)
{
    char name[64];
    std::snprintf(name, sizeof name, "/tex%u.dds", index);
    if (argon::gamePathExists(folder + name))
    {
        return folder + name;
    }
    std::snprintf(name, sizeof name, "/lotex%u.dds", index);
    return folder + name;
}

void readTextures(const std::string &folder, ModelSource &src)
{
    for (uint32_t i = 0; i < src.cmm.numTextures; ++i)
    {
        argon::TextureData td;   // none keeps the indices aligned
        try
        {
            td = argon::loadDdsFrames(texturePath(folder, i), src.cmm.texInfo[i].frames);
        }
        catch (const std::runtime_error &e)
        {
            src.warnings.push_back(e.what());
        }
        if (td.cube)
        {
            td.layers = 1;            // material textures are 2D (face 0)
            td.cube = false;
        }
        src.textures.push_back(std::move(td));
    }
}

// the mesh's placement: on its bone unless skinned (skinned vertices are already in model space)
glm::mat4 meshPlacement(const argon::Cmm &cmm, const argon::CmmMesh &mesh)
{
    glm::mat4 placement(1.f);
    if (mesh.bone >= 0 && !mesh.skinned)
    {
        cmm.boneWorld(mesh.bone, glm::value_ptr(placement));
    }
    return placement;
}

uint32_t d3dColour(const argon::Vertex &vertex)
{
    return uint32_t(vertex.rgba[3]) << 24 | uint32_t(vertex.rgba[0]) << 16 | uint32_t(vertex.rgba[1]) << 8 | vertex.rgba[2];
}

// the vertex the hover samples (0x5f0d7): strip index tri + 2 (the swap in Cmm::geometry leaves it last), list t[0]
uint32_t sampledColour(const argon::CmmMesh &mesh, const std::vector<argon::Vertex> &verts, const uint32_t *triangle)
{
    const bool strip = (mesh.flags & argon::original::mesh_flags::kTriangleStrip) != 0;
    return d3dColour(verts[strip ? triangle[2] : triangle[0]]);
}

void addCollision(const argon::CmmMesh &mesh, const std::vector<argon::Vertex> &verts, const std::vector<uint32_t> &indices, const glm::mat4 &placement,
                  ModelSource &src)
{
    for (size_t k = 0; k + 2 < indices.size(); k += 3)
    {
        for (size_t corner = 0; corner < 3; ++corner)
        {
            const float *position = verts[indices[k + corner]].pos;
            src.collTris.push_back(glm::vec3(placement * glm::vec4(position[0], position[1], position[2], 1.f)));
        }
        src.collSurf.push_back({uint8_t(std::min(mesh.surfaceType, 4u)), mesh.surfaceReset == 1, sampledColour(mesh, verts, &indices[k])});
    }
}

void measureMesh(const std::vector<argon::Vertex> &verts, const glm::mat4 &placement, MeshRange &range)
{
    for (const argon::Vertex &vertex : verts)
    {
        const glm::vec3 p(vertex.pos[0], vertex.pos[1], vertex.pos[2]);
        const glm::vec3 placed = glm::vec3(placement * glm::vec4(p, 1.f));
        range.placedMin = glm::min(range.placedMin, placed);
        range.placedMax = glm::max(range.placedMax, placed);
        range.localMin = glm::min(range.localMin, p);
        range.localMax = glm::max(range.localMax, p);
    }
}

// the bump effects' prepares (0x7d5c0 / 0x7ca40): their vertices carry the tangent basis (bump_basis.hpp)
void addEffectBasis(const argon::Cmm &cmm, const argon::CmmMesh &mesh, const std::vector<uint32_t> &indices, std::vector<argon::Vertex> &verts)
{
    if (argon::usesDiffuseSpecularBump(cmm, mesh))
    {
        argon::storeBumpBasis(argon::buildBumpBasis(verts, indices, argon::bumpBasisUvSet(cmm, mesh)), verts);
    }
}

// each mesh's geometry appended to the model's arrays (meshes without triangles keep an empty range)
void readMeshes(ModelSource &src)
{
    std::vector<argon::Vertex> verts;
    std::vector<uint32_t> indices;
    for (const argon::CmmMesh &mesh : src.cmm.meshes)
    {
        MeshRange range;
        src.cmm.geometry(mesh, verts, indices);
        if (!indices.empty())
        {
            const glm::mat4 placement = meshPlacement(src.cmm, mesh);
            if (mesh.collidable)
            {
                addCollision(mesh, verts, indices, placement, src);
            }
            measureMesh(verts, placement, range);
            addEffectBasis(src.cmm, mesh, indices, verts);
            range.firstIndex = uint32_t(src.indices.size());
            range.indexCount = uint32_t(indices.size());
            range.vertexOffset = int32_t(src.verts.size());
            src.verts.insert(src.verts.end(), verts.begin(), verts.end());
            src.indices.insert(src.indices.end(), indices.begin(), indices.end());
        }
        src.meshes.push_back(range);
    }
}

}  // namespace

ModelSource readModel(const std::string &folder)
{
    ModelSource src;
    try
    {
        src.cmm.load(folder + "/model.cmm");
    }
    catch (const std::exception &e)   // the loader's own range and format errors too
    {
        src.warnings.push_back(e.what());
        return src;
    }
    src.loaded = true;
    readTextures(folder, src);
    readMeshes(src);
    return src;
}
