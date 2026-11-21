// Reads a .cmm file's sections in file order (docs\models\README.md): header, mesh records, materials, bones, surface blocks,
// vertex and index data, the texture table, the effect records and their programs, the animation tracks.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <stdexcept>

#include "model/cmm.hpp"
#include "model/cmm_tracks.hpp"
#include "original/cmm_records.hpp"
#include "original/record_read.hpp"

namespace argon
{

namespace
{

using original::readAt;
namespace mesh_flags = original::mesh_flags;

const uint32_t kMaxEffects = 64;

uint32_t morphStride(uint32_t kind)
{
    static const std::array<std::pair<uint32_t, uint32_t>, 5> kStrides = {{{7, 0x1c}, {3, 0x18}, {5, 0x10}, {1, 0x0c}, {4, 0x04}}};
    for (const auto &[k, stride] : kStrides)
    {
        if (k == kind)
        {
            return stride;
        }
    }
    return 0;
}

size_t alignTo16(size_t offset)
{
    return (offset + 15) & ~size_t(15);
}

class CmmLoader
{
  public:
    explicit CmmLoader(Cmm &cmm) : cmm_(cmm), data_(cmm.data)
    {
    }

    void load()
    {
        readHeader();
        readMeshRecords();
        readMaterials();
        const size_t bonesAt = skipBones();
        readSurfaceBlocks();
        readVertexData();
        readIndexData();
        if (cursor_ > data_.size())
        {
            throw std::runtime_error("mesh data runs past end of file");
        }
        const size_t effectsAt = cursor_ + readTextureTable();
        readAnimationTracks(readEffects(effectsAt));
        readBones(bonesAt);
    }

  private:
    void readHeader()
    {
        if (data_.size() < sizeof(original::CmmHeader))
        {
            throw std::runtime_error("file too small");
        }
        header_ = readAt<original::CmmHeader>(data_, 0);
        cmm_.version = header_.version;
        if (cmm_.version != original::kCmmVersion)
        {
            throw std::runtime_error("unsupported cmm version " + std::to_string(cmm_.version));
        }
        cmm_.numTextures = header_.textureCount;
        cmm_.flags = header_.flags;
        cursor_ = sizeof(original::CmmHeader);
    }

    void readMeshRecords()
    {
        cmm_.meshes.resize(header_.meshCount);
        for (uint32_t i = 0; i < header_.meshCount; ++i)
        {
            const auto record = readAt<original::CmmMesh>(data_, cursor_ + i * sizeof(original::CmmMesh));
            CmmMesh &mesh = cmm_.meshes[i];
            mesh.index = int(i);
            mesh.bone = record.bone;
            mesh.numVerts = record.vertexCount;
            mesh.flags = record.flags;
            mesh.numFaces = record.faceCount;
            for (uint32_t k = 0; k < record.stageCount && k < std::size(record.textures); ++k)
            {
                mesh.tex.push_back(record.textures[k]);
            }
            mesh.vtype = record.vertexType;
            mesh.material = record.material;
            std::copy(std::begin(record.sortPoint), std::end(record.sortPoint), mesh.sortPoint);
            mesh.cubeIndex = record.cubeMap;
        }
        cursor_ += size_t(header_.meshCount) * sizeof(original::CmmMesh);
    }

    void readMaterials()
    {
        cmm_.materialsOfs = cursor_;
        cmm_.numMaterials = std::max(0, int32_t(header_.materialCount));
        cursor_ += size_t(cmm_.numMaterials) * sizeof(original::CmmMaterial);
        for (auto &mesh : cmm_.meshes)
        {
            if (mesh.material >= 0 && mesh.material < cmm_.numMaterials)
            {
                mesh.effect = cmm_.materialRecord(mesh.material).effect;
            }
        }
    }

    // the bone records (16-byte aligned) are read last, once the animation tracks they refer to are known
    size_t skipBones()
    {
        if (int32_t(header_.boneCount) <= 0)
        {
            return 0;
        }
        cursor_ = alignTo16(cursor_);
        const size_t at = cursor_;
        cursor_ += size_t(header_.boneCount) * sizeof(original::CmmBone);
        return at;
    }

    void readSurfaceBlocks()
    {
        for (auto &mesh : cmm_.meshes)
        {
            if (mesh.flags & mesh_flags::kHasSurfaceBlock)
            {
                cursor_ += readSurfaceBlock(mesh);
            }
        }
    }

    size_t readSurfaceBlock(CmmMesh &mesh) const
    {
        const auto block = readAt<original::CmmSurfaceHeader>(data_, cursor_);
        const auto collision = readAt<original::CmmSurfaceCollision>(data_, cursor_ + sizeof(original::CmmSurfaceHeader));
        mesh.hasSurface = true;
        mesh.blendDesc = block.blend;
        mesh.surfaceFlags = block.flags;
        mesh.surfaceSetup = block.setup;
        mesh.surfaceType = collision.surfaceType;
        mesh.surfaceReset = collision.craftReset;
        const size_t lightingOffset = original::lightingOffset(block.setup);
        if (lightingOffset && block.size >= lightingOffset + sizeof(original::CmmSurfaceLighting))
        {
            const auto lighting = readAt<original::CmmSurfaceLighting>(data_, cursor_ + lightingOffset);
            mesh.surfaceColour = lighting.colour;
            mesh.occlRadius = lighting.occlusionRadius;
            mesh.occlPoints = lighting.occlusionPointCount;
            mesh.billboardFactor = lighting.billboardFactor;
            mesh.fogMode = lighting.fogMode;
            mesh.fogColour = lighting.fogColour;
            mesh.fogDensity = lighting.fogDensity;
            mesh.fogRange = lighting.fogRange;
        }
        mesh.collidable = (mesh.surfaceFlags & original::surface_flags::kCollidable) != 0;
        mesh.drawn = (mesh.surfaceFlags & original::surface_flags::kDrawn) != 0;
        if (block.size == 0)
        {
            throw std::runtime_error("bad surface block");
        }
        return block.size;
    }

    void readVertexData()
    {
        for (auto &mesh : cmm_.meshes)
        {
            if (!(mesh.flags & mesh_flags::kSkinned))
            {
                skipPlainVertices(mesh);
            }
            if (mesh.flags & mesh_flags::kMorph)
            {
                readMorphStreams(mesh);
            }
            if (mesh.flags & mesh_flags::kSkinned)
            {
                readSkinnedVertices(mesh);
            }
        }
    }

    void skipPlainVertices(CmmMesh &mesh)
    {
        if (mesh.vtype < 0 || mesh.vtype > 7)
        {
            throw std::runtime_error("unknown vertex type " + std::to_string(mesh.vtype));
        }
        mesh.vertOfs = cursor_;
        cursor_ += size_t(mesh.numVerts) * original::vertexSize(mesh.vtype);
    }

    void readMorphStreams(CmmMesh &mesh)
    {
        const auto morph = readAt<original::CmmMorphHeader>(data_, cursor_);
        cursor_ += sizeof(original::CmmMorphHeader) + size_t(morph.streamCount) * 8;
        mesh.morph.kind = morph.kind;
        mesh.morph.streams = morph.streamCount;
        mesh.morph.streamsPerSecond = morph.streamsPerSecond;
        mesh.morph.stride = morphStride(morph.kind);
        mesh.morph.firstStreamOfs = cursor_;
        cursor_ += size_t(morph.streamCount) * size_t(mesh.numVerts) * mesh.morph.stride;
        if (cursor_ > data_.size())
        {
            throw std::runtime_error("morph streams past the end of the file");
        }
    }

    void readSkinnedVertices(CmmMesh &mesh)
    {
        const auto skin = readAt<original::CmmSkinHeader>(data_, cursor_);
        mesh.skinBones.clear();
        for (uint32_t k = 0; k < skin.boneCount && k < std::size(skin.bones); ++k)
        {
            mesh.skinBones.push_back(int(skin.bones[k]));
        }
        cursor_ += sizeof(original::CmmSkinHeader);
        mesh.skinned = true;
        mesh.vertOfs = cursor_;
        mesh.skinStride = uint32_t(original::skinnedVertexSize(skin.boneCount));
        cursor_ += size_t(mesh.numVerts) * mesh.skinStride;
    }

    void readIndexData()
    {
        for (auto &mesh : cmm_.meshes)
        {
            mesh.idxOfs = cursor_;
            cursor_ += (mesh.flags & mesh_flags::kTriangleStrip) ? 4 + size_t(mesh.numFaces) * 2 : size_t(mesh.numFaces) * 6;
        }
    }

    // returns the table's size (0 without one)
    size_t readTextureTable()
    {
        cmm_.texInfo.assign(cmm_.numTextures, CmmTexInfo{});
        const bool hasTable = (cmm_.flags & original::cmm_flags::kHasTextureTable) && header_.textureTable;
        const size_t size = hasTable ? size_t(cmm_.numTextures) * sizeof(original::CmmTextureEntry) : 0;
        if (!hasTable || cursor_ + size > data_.size())
        {
            return size;
        }
        for (uint32_t i = 0; i < cmm_.numTextures; ++i)
        {
            const auto entry = readAt<original::CmmTextureEntry>(data_, cursor_ + i * sizeof(original::CmmTextureEntry));
            cmm_.texInfo[i].type = entry.type;
            cmm_.texInfo[i].frames = std::max(1u, entry.frameCount);
        }
        return size;
    }

    // the effect records (selected per material by CmmMaterial::effect), then their vertex shader programs; returns
    // the offset after the programs (where the animation tracks start, 0xb6070)
    size_t readEffects(size_t at)
    {
        const uint32_t count = header_.effectCount;
        constexpr size_t kEffectSize = sizeof(original::CmmEffect);
        if (count >= kMaxEffects)
        {
            return at;
        }
        for (uint32_t i = 0; i < count && at + (i + 1) * kEffectSize <= data_.size(); ++i)
        {
            const char *name = reinterpret_cast<const char *>(data_.data() + at + i * kEffectSize);
            cmm_.effects.push_back(fixedString(name, sizeof(original::CmmEffect::name)));
        }
        size_t end = at + size_t(count) * kEffectSize;
        for (uint32_t i = 0; i < count; ++i)
        {
            end += readAt<original::CmmEffect>(data_, at + i * kEffectSize).programSize;
        }
        return end;
    }

    // the track records, then every track's keys in record order; a broken track drops all of them
    void readAnimationTracks(size_t at)
    {
        argon::readAnimationTracks(data_, AnimationBlock{at, header_.animationTrackCount}, cmm_.tracks);
    }

    void readBones(size_t at)
    {
        const int count = std::max(0, int32_t(header_.boneCount));
        cmm_.bones.resize(size_t(count));
        for (int i = 0; i < count; ++i)
        {
            const auto record = readAt<original::CmmBone>(data_, at + size_t(i) * sizeof(original::CmmBone));
            CmmBone &bone = cmm_.bones[size_t(i)];
            std::copy(std::begin(record.local), std::end(record.local), bone.local);
            bone.name = fixedString(record.name, sizeof record.name);
            bone.parent = record.parent;
            bone.track = record.animationTrack < int(cmm_.tracks.size()) ? record.animationTrack : -1;
            std::copy(std::begin(record.rowScale), std::end(record.rowScale), bone.scale);
        }
        for (auto &mesh : cmm_.meshes)
        {
            if (mesh.bone >= count)
            {
                mesh.bone = -1;
            }
        }
    }

    Cmm &cmm_;
    const std::vector<uint8_t> &data_;
    original::CmmHeader header_{};
    size_t cursor_ = 0;
};

} // namespace

void Cmm::load(const std::string &path)
{
    if (!readFile(path, data))
    {
        throw std::runtime_error("cannot read " + path);
    }
    CmmLoader(*this).load();
}

} // namespace argon
