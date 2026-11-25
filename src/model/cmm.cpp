#include <array>
#include "model/cmm.hpp"
#include "files/game_path.hpp"
#include <glm/gtc/type_ptr.hpp>
#include "original/cmm_records.hpp"
#include "original/record_read.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>

namespace argon
{

using original::readAt;
namespace mesh_flags = original::mesh_flags;

// a skinned vertex needs at least two bones
constexpr uint32_t kTwoBoneVertexSize = uint32_t(original::skinnedVertexSize(2));

static void unpackColour(uint32_t colour, uint8_t rgba[4])
{   // D3DCOLOR 0xAARRGGBB
    rgba[0] = uint8_t(colour >> 16);
    rgba[1] = uint8_t(colour >> 8);
    rgba[2] = uint8_t(colour);
    rgba[3] = uint8_t(colour >> 24);
}

// the type's UV pairs at offset into the four sets, the last pair repeated
static void readUvPairs(const std::vector<uint8_t> &bytes, size_t offset, int vertexType, Vertex &vertex)
{
    const int count = original::uvPairCount(vertexType);
    for (int k = 0; k < 4; ++k)
    {
        const size_t pairOffset = offset + size_t(std::min(k, count - 1)) * sizeof(original::CmmUvPair);
        const auto uv = readAt<original::CmmUvPair>(bytes, pairOffset);
        vertex.uv[k][0] = uv.u;
        vertex.uv[k][1] = uv.v;
    }
}

// vertex types 0..7; the packed types (4..7) get a zero normal, built from the triangles afterwards
static Vertex readUnskinnedVertex(const std::vector<uint8_t> &bytes, size_t offset, int vertexType)
{
    Vertex vertex{};
    if (original::isPackedVertexType(vertexType))
    {
        const auto record = readAt<original::CmmPackedVertex>(bytes, offset);
        std::copy(std::begin(record.position), std::end(record.position), vertex.pos);
        unpackColour(record.colour, vertex.rgba);
        readUvPairs(bytes, offset + sizeof(record), vertexType, vertex);
        return vertex;
    }
    const auto record = readAt<original::CmmPlainVertex>(bytes, offset);
    std::copy(std::begin(record.position), std::end(record.position), vertex.pos);
    std::copy(std::begin(record.normal), std::end(record.normal), vertex.nrm);
    unpackColour(record.colour, vertex.rgba);
    readUvPairs(bytes, offset + sizeof(record), vertexType, vertex);
    return vertex;
}

namespace
{

// weighted vertices (flag 0x40000, vtype = bones + 6): n positions in the bones' spaces, one normal, D3DCOLOR, uv,
// n weights. Skinned here in the rest pose (bone world = local x parent chain): pos = sum w_k (W_k pos_k), normal =
// sum w_k (W_k normal); the draw then uses an identity model matrix. None without a skin.
std::vector<Vertex> restPoseVertices(const Cmm &model, const CmmMesh &mesh)
{
    if (mesh.skinBones.empty() || mesh.skinStride < kTwoBoneVertexSize)
    {
        return {};
    }
    std::vector<float> world(model.bones.size() * 16);
    for (size_t b = 0; b < model.bones.size(); ++b)
    {
        model.boneWorld(int(b), &world[b * 16]);
    }
    std::vector<Vertex> vertices(mesh.numVerts);
    model.skin(mesh, world.data(), vertices.data());
    return vertices;
}

// No axis flip: the game's data is effectively right-handed (it renders with a mirrored projection; see
// docs\models\README.md "Coordinate system").
std::vector<Vertex> plainVertices(const std::vector<uint8_t> &data, const CmmMesh &mesh)
{
    const size_t stride = original::vertexSize(mesh.vtype);
    std::vector<Vertex> vertices(mesh.numVerts);
    for (uint32_t i = 0; i < mesh.numVerts; ++i)
    {
        vertices[i] = readUnskinnedVertex(data, mesh.vertOfs + size_t(i) * stride, mesh.vtype);
    }
    return vertices;
}

// a triangle strip of faces + 2 indices, without its degenerate and out-of-range triangles, every odd one turned
std::vector<uint32_t> stripTriangles(const std::vector<uint8_t> &data, const CmmMesh &mesh)
{
    const uint32_t count = mesh.numFaces + 2;
    std::vector<uint16_t> strip(count);
    std::memcpy(strip.data(), data.data() + mesh.idxOfs, count * 2);
    std::vector<uint32_t> indices;
    for (uint32_t i = 0; i + 2 < count; ++i)
    {
        uint16_t a = strip[i], b = strip[i + 1], c = strip[i + 2];
        if (a == b || b == c || a == c)
        {
            continue;
        }
        if (a >= mesh.numVerts || b >= mesh.numVerts || c >= mesh.numVerts)
        {
            continue;
        }
        if (i & 1)
        {
            std::swap(a, b);
        }
        indices.insert(indices.end(), {a, b, c});
    }
    return indices;
}

// a triangle list, without its out-of-range triangles
std::vector<uint32_t> listTriangles(const std::vector<uint8_t> &data, const CmmMesh &mesh)
{
    std::vector<uint32_t> indices;
    for (uint32_t f = 0; f < mesh.numFaces; ++f)
    {
        uint16_t triangle[3];
        std::memcpy(triangle, data.data() + mesh.idxOfs + f * 6, 6);
        if (triangle[0] >= mesh.numVerts || triangle[1] >= mesh.numVerts || triangle[2] >= mesh.numVerts)
        {
            continue;
        }
        indices.insert(indices.end(), {triangle[0], triangle[1], triangle[2]});
    }
    return indices;
}

// the packed types' normals: area-weighted face normals (front faces are clockwise), straight up where none
void addFaceNormals(std::vector<Vertex> &verts, const std::vector<uint32_t> &idx)
{
    for (size_t i = 0; i + 2 < idx.size(); i += 3)
    {
        const float *a = verts[idx[i]].pos;
        const float *b = verts[idx[i + 1]].pos;
        const float *c = verts[idx[i + 2]].pos;
        const float e1[3] = {c[0] - a[0], c[1] - a[1], c[2] - a[2]}, e2[3] = {b[0] - a[0], b[1] - a[1], b[2] - a[2]};
        const float normal[3] = {e1[1] * e2[2] - e1[2] * e2[1], e1[2] * e2[0] - e1[0] * e2[2], e1[0] * e2[1] - e1[1] * e2[0]};
        for (int k = 0; k < 3; ++k)
        {
            for (int j = 0; j < 3; ++j)
            {
                verts[idx[i + k]].nrm[j] += normal[j];
            }
        }
    }
    for (auto &vertex : verts)
    {
        const float length = std::sqrt(vertex.nrm[0] * vertex.nrm[0] + vertex.nrm[1] * vertex.nrm[1] + vertex.nrm[2] * vertex.nrm[2]);
        if (length > 1e-12f)
        {
            for (float &x : vertex.nrm)
            {
                x /= length;
            }
        }
        else
        {
            vertex.nrm[0] = 0;
            vertex.nrm[1] = 1;
            vertex.nrm[2] = 0;
        }
    }
}

}  // namespace

void Cmm::geometry(const CmmMesh &mesh, std::vector<Vertex> &verts, std::vector<uint32_t> &idx) const
{
    verts.clear();
    idx.clear();
    if (mesh.numVerts == 0)
    {
        return;
    }
    verts = mesh.skinned ? restPoseVertices(*this, mesh) : plainVertices(data, mesh);
    if (verts.empty())
    {
        return;
    }
    idx = (mesh.flags & 1) ? stripTriangles(data, mesh) : listTriangles(data, mesh);
    if (!mesh.skinned && original::isPackedVertexType(mesh.vtype))
    {
        addFaceNormals(verts, idx);
    }
}

std::vector<CmmStage> Cmm::stages(const CmmMesh &mesh) const
{
    std::vector<CmmStage> out;
    if (mesh.material < 0 || mesh.material >= numMaterials)
    {
        return out;
    }
    const original::CmmMaterial material = materialRecord(mesh.material);
    size_t count = std::min<size_t>(mesh.tex.size(), std::size(material.stages));
    // an effect: mesh_apply_material_stages (0xba660) skips the env-map setup (0xbb110) for every stage, so a 0x8
    // stage keeps its own UV set (e.g. the road detail normal maps on bracken)
    bool noEnv = material.effect >= 0;
    for (size_t s = 0; s < count; ++s)
    {
        const original::CmmStage &record = material.stages[s];
        CmmStage st;
        st.flags = record.flags;
        if (noEnv)
        {
            st.flags &= ~8u;
        }
        st.tex = mesh.tex[s];                       // same as CmmStage::texture
        st.texcoord = record.texCoordIndex & 3;
        st.colorCode = record.colourOp;
        st.alphaCode = record.alphaOp;
        st.rate = record.framesPerSecond;
        st.scrollU = record.scrollU;
        st.scrollV = record.scrollV;
        st.offU = record.offsetU;
        st.offV = record.offsetV;
        out.push_back(st);
    }
    return out;
}

original::CmmMaterial Cmm::materialRecord(int index) const
{
    return readAt<original::CmmMaterial>(data, materialsOfs + size_t(index) * sizeof(original::CmmMaterial));
}

void Cmm::boneWorld(int bone, float out[16]) const
{
    // world = local * parent_world (row-vector); returned as column-major for column vectors.
    float world[16];
    for (int i = 0; i < 16; ++i)
    {
        world[i] = (i % 5 == 0) ? 1.f : 0.f;
    }
    int guard = 0;
    for (int b = bone; b >= 0 && b < int(bones.size()) && guard < 256; b = bones[b].parent, ++guard)
    {
        float product[16];
        const float *local = bones[b].local;    // W = W * L  (child first, then parents)
        for (int r = 0; r < 4; ++r)
        {
            for (int c = 0; c < 4; ++c)
            {
                float sum = 0;
                for (int k = 0; k < 4; ++k)
                {
                    sum += world[r * 4 + k] * local[k * 4 + c];
                }
                product[r * 4 + c] = sum;
            }
        }
        std::memcpy(world, product, sizeof(world));
    }
    // row-major row-vector W -> column-major column-vector: same 16 floats in order
    std::memcpy(out, world, sizeof(world));
}

bool Cmm::animated() const
{
    for (const CmmMesh &mesh : meshes)
    {
        if (mesh.morph.streams > 0)
        {
            return true;
        }
    }
    if (tracks.empty())
    {
        return false;
    }
    for (auto &bone : bones)
    {
        if (bone.track >= 0)
        {
            return true;
        }
    }
    return false;
}

namespace
{
struct KeyPair
{
    uint32_t k0 = 0, k1 = 0;
    float u = 0.f;   // the weight of k1
};

// frame = t x the track's keys per second; looping: keys floor(frame) % keys and the next one (each track on its own
// key count); play once (bone flag 0x400): key 0 while the frame is <= 0, clamped to the last key
KeyPair keysAt(const CmmTrack &tr, float seconds, bool once)
{
    const float frame = seconds * tr.fps, fl = std::floor(frame);
    KeyPair keys;
    keys.u = std::clamp(frame - fl, 0.f, 1.f);
    if (once)
    {
        const long long last = (long long)tr.keys - 1;
        keys.k0 = frame <= 0.f ? 0u : uint32_t(std::min((long long)fl, last));
        keys.k1 = frame <= 0.f ? 0u : uint32_t(std::min((long long)fl + 1, last));
        return keys;
    }
    const long long ki = (long long)fl, nk = (long long)tr.keys;
    keys.k0 = uint32_t(((ki % nk) + nk) % nk);
    keys.k1 = (keys.k0 + 1) % tr.keys;
    return keys;
}

// slerp (0xf360b) of two quaternions, renormalised
void slerp(const float *from, const float *bp, float u, float *out)
{
    float bq[4] = {bp[0], bp[1], bp[2], bp[3]};
    float cosine = from[0] * bq[0] + from[1] * bq[1] + from[2] * bq[2] + from[3] * bq[3];
    if (cosine < 0)
    {
        cosine = -cosine;
        for (float &component : bq)
        {
            component = -component;
        }
    }
    float s0 = 1.f - u, s1 = u;
    if (cosine < 0.9995f)
    {
        const float th = std::acos(std::min(cosine, 1.f)), st = std::sin(th);
        s0 = std::sin((1.f - u) * th) / st;
        s1 = std::sin(u * th) / st;
    }
    for (int c = 0; c < 4; ++c)
    {
        out[c] = s0 * from[c] + s1 * bq[c];
    }
    const float length = std::sqrt(out[0] * out[0] + out[1] * out[1] + out[2] * out[2] + out[3] * out[3]);
    if (length > 1e-12f)
    {
        for (int c = 0; c < 4; ++c)
        {
            out[c] /= length;
        }
    }
}

// the quaternion as a matrix (0xf3035) with its rows scaled by bone +0x11c..+0x124
void writeRotation(const float *quaternion, const float *rowScale, float *matrix)
{
    const float x = quaternion[0], y = quaternion[1], z = quaternion[2], w = quaternion[3];
    const float rotation[9] = {1 - 2 * (y * y + z * z),
                        2 * (x * y + z * w),
                        2 * (x * z - y * w),
                        2 * (x * y - z * w),
                        1 - 2 * (x * x + z * z),
                        2 * (y * z + x * w),
                        2 * (x * z + y * w),
                        2 * (y * z - x * w),
                        1 - 2 * (x * x + y * y)};
    for (int r = 0; r < 3; ++r)
    {
        for (int c = 0; c < 3; ++c)
        {
            matrix[r * 4 + c] = rotation[r * 3 + c] * rowScale[r];
        }
    }
}

// a bone's local matrix from its keys: rotation = slerp, translation = lerp of the positions, scale keys multiply
// the rows; a stream the track lacks keeps the stored local's
void sampleBone(const CmmBone &bone, const BoneKeys &driver, float *matrix)
{
    std::memcpy(matrix, bone.local, 64);
    if (!driver.track || driver.track->keys == 0)
    {
        return;
    }
    const CmmTrack &tr = *driver.track;
    const KeyPair keys = keysAt(tr, driver.seconds, driver.once);
    const float u = keys.u, v = 1.f - u;
    if (!tr.quat.empty())
    {
        float quaternion[4];
        slerp(&tr.quat[size_t(keys.k0) * 4], &tr.quat[size_t(keys.k1) * 4], u, quaternion);
        writeRotation(quaternion, bone.scale, matrix);
    }
    if (!tr.pos.empty())
    {
        for (int c = 0; c < 3; ++c)
        {
            matrix[12 + c] = u * tr.pos[size_t(keys.k1) * 3 + c] + v * tr.pos[size_t(keys.k0) * 3 + c];
        }
    }
    if (!tr.scl.empty())
    {
        for (int r = 0; r < 3; ++r)
        {
            const float scale = u * tr.scl[size_t(keys.k1) * 3 + r] + v * tr.scl[size_t(keys.k0) * 3 + r];
            for (int c = 0; c < 3; ++c)
            {
                matrix[r * 4 + c] *= scale;
            }
        }
    }
}

// row-vector W = L x P
void multiplyRowMajor(const float *local, const float *parent, float *world)
{
    for (int r = 0; r < 4; ++r)
    {
        for (int c = 0; c < 4; ++c)
        {
            float sum = 0;
            for (int k = 0; k < 4; ++k)
            {
                sum += local[r * 4 + k] * parent[k * 4 + c];
            }
            world[r * 4 + c] = sum;
        }
    }
}

// pose()'s working storage, kept between calls so a pose allocates nothing once its sizes have been seen
struct PoseScratch
{
    std::vector<float> local;              // each bone's local matrix
    std::vector<float> parentOfChildren;   // the world each bone's children hang off
    std::vector<uint8_t> done;
    std::vector<int> chain;                // a bone and its not yet posed parents
};

PoseScratch &poseScratch()
{
    thread_local PoseScratch scratch;
    return scratch;
}

// the turned world replaces the posed one; with 0x2000 the children hang off the turned bone as well
void aimPosedBone(const CmmBone &bone, const BoneAim &aim, float *world, float *parentOfChildren)
{
    if (aim.kind == AimKind::None)
    {
        return;
    }
    const glm::mat4 aimed = aimBone(glm::make_mat4(world), aim, glm::vec3(bone.scale[0], bone.scale[1], bone.scale[2]) / aim.modelScale);
    std::memcpy(world, glm::value_ptr(aimed), 64);
    if (aim.kind == AimKind::TurnsChildren)
    {
        std::memcpy(parentOfChildren, world, 64);
    }
}
} // namespace

// 0xb8630 (bone flags are 0 in every front-end character): each bone with a track sampled at t
void Cmm::pose(float seconds, std::vector<float> &world16) const
{
    thread_local std::vector<BoneKeys> drivers;
    ownTracks(seconds, drivers);
    pose(drivers, world16);
}

std::vector<BoneKeys> Cmm::ownTracks(float seconds) const
{
    std::vector<BoneKeys> drivers;
    ownTracks(seconds, drivers);
    return drivers;
}

void Cmm::ownTracks(float seconds, std::vector<BoneKeys> &drivers) const
{
    drivers.assign(bones.size(), BoneKeys{});
    for (size_t i = 0; i < bones.size(); ++i)
    {
        const int track = bones[i].track;
        if (track >= 0 && track < int(tracks.size()))
        {
            drivers[i] = BoneKeys{&tracks[size_t(track)], seconds, false};
        }
    }
}

// world = local x parent world
void Cmm::pose(const std::vector<BoneKeys> &drivers, std::vector<float> &world16) const
{
    pose(drivers, {}, world16);
}

// a bone's children take its world after the aim, or with 0x4000 the one before (0xb8630 recurses with +0xc0)
void Cmm::pose(const std::vector<BoneKeys> &drivers, const std::vector<BoneAim> &aims, std::vector<float> &world16) const
{
    PoseScratch &scratch = poseScratch();
    const size_t size = bones.size();
    std::vector<float> &local = scratch.local;
    local.resize(size * 16);
    for (size_t i = 0; i < size; ++i)
    {
        sampleBone(bones[i], i < drivers.size() ? drivers[i] : BoneKeys{}, &local[i * 16]);
    }
    world16.assign(size * 16, 0.f);
    std::vector<float> &parentOfChildren = scratch.parentOfChildren;
    parentOfChildren.assign(size * 16, 0.f);
    std::vector<uint8_t> &done = scratch.done;
    done.assign(size, 0);
    std::vector<int> &chain = scratch.chain;
    for (size_t i = 0; i < size; ++i)
    {
        chain.clear();
        for (int b = int(i); b >= 0 && b < int(size) && !done[size_t(b)] && chain.size() < 256; b = bones[size_t(b)].parent)
        {
            chain.push_back(b);
        }
        for (size_t c = chain.size(); c-- > 0;)
        {
            const int bone = chain[c], p = bones[size_t(bone)].parent;
            float *world = &world16[size_t(bone) * 16];
            const float *Lb = &local[size_t(bone) * 16];
            if (p < 0 || p >= int(size) || !done[size_t(p)])
            {
                std::memcpy(world, Lb, 64);
            }
            else
            {
                multiplyRowMajor(Lb, &parentOfChildren[size_t(p) * 16], world);
            }
            std::memcpy(&parentOfChildren[size_t(bone) * 16], world, 64);
            if (size_t(bone) < aims.size())
            {
                aimPosedBone(bones[size_t(bone)], aims[size_t(bone)], world, &parentOfChildren[size_t(bone) * 16]);
            }
            done[size_t(bone)] = 1;
        }
    }
}

// weighted vertices (flag 0x40000, vtype = bones + 6): n positions in the bones' spaces, one normal, D3DCOLOR,
// uv, n weights: pos = sum w_k (W_k pos_k), normal = sum w_k (W_k normal)
void Cmm::skin(const CmmMesh &mesh, const float *world16, Vertex *out) const
{
    size_t nb = mesh.skinBones.size();
    if (nb < 1 || nb > 8 || mesh.skinStride < kTwoBoneVertexSize)
    {
        return;
    }
    if (mesh.vertOfs + size_t(mesh.numVerts) * mesh.skinStride > data.size())
    {
        return;
    }
    static const float identity[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    const float *boneMatrices[8];
    for (size_t k = 0; k < nb; ++k)
    {
        int bone = mesh.skinBones[k];
        boneMatrices[k] = (bone >= 0 && bone < int(bones.size())) ? world16 + size_t(bone) * 16 : identity;
    }
    for (uint32_t i = 0; i < mesh.numVerts; ++i)
    {
        const size_t vertexOffset = mesh.vertOfs + size_t(i) * mesh.skinStride;
        float pk[8][3], w[8];
        std::memcpy(pk, data.data() + vertexOffset, nb * sizeof(original::CmmSkinnedVertexBone::position));
        const auto shared = readAt<original::CmmSkinnedVertexShared>(data, vertexOffset + original::skinnedSharedOffset(nb));
        std::memcpy(w, data.data() + vertexOffset + original::skinnedWeightsOffset(nb), nb * sizeof(original::CmmSkinnedVertexBone::weight));
        const float *normal = shared.normal;
        Vertex &vertex = out[i];
        float position[3] = {0, 0, 0}, nn[3] = {0, 0, 0}, wsum = 0;
        for (size_t k = 0; k < nb; ++k)
        {
            const float *boneMatrix = boneMatrices[k];   // column-major
            float x = pk[k][0], y = pk[k][1], z = pk[k][2];
            for (int j = 0; j < 3; ++j)
            {
                position[j] += w[k] * (boneMatrix[j] * x + boneMatrix[4 + j] * y + boneMatrix[8 + j] * z + boneMatrix[12 + j]);
                nn[j] += w[k] * (boneMatrix[j] * normal[0] + boneMatrix[4 + j] * normal[1] + boneMatrix[8 + j] * normal[2]);
            }
            wsum += w[k];
        }
        if (wsum > 1e-6f && std::fabs(wsum - 1.f) > 1e-3f)
        {
            for (float &component : position)
            {
                component /= wsum;
            }
        }
        if (!(mesh.flags & mesh_flags::kWeightedNormals) && mesh.bone >= 0 && mesh.bone < int(bones.size()))
        {
            // without mesh flag 0x800 (the faces) the one normal is in the mesh's own bone space, not the weighted
            // bones' (matches the triangles to 0.97..0.998; blending it through the jaw, turned ~90 deg at rest,
            // lit the chin wrong)
            const float *boneMatrix = world16 + size_t(mesh.bone) * 16;
            for (int j = 0; j < 3; ++j)
            {
                nn[j] = boneMatrix[j] * normal[0] + boneMatrix[4 + j] * normal[1] + boneMatrix[8 + j] * normal[2];
            }
        }
        float length = std::sqrt(nn[0] * nn[0] + nn[1] * nn[1] + nn[2] * nn[2]);
        for (int j = 0; j < 3; ++j)
        {
            vertex.pos[j] = position[j];
            vertex.nrm[j] = length > 1e-9f ? nn[j] / length : (j == 1 ? 1.f : 0.f);
        }
        unpackColour(shared.colour, vertex.rgba);
        for (int k = 0; k < 4; ++k)
        {
            vertex.uv[k][0] = shared.uv.u;
            vertex.uv[k][1] = shared.uv.v;
        }
    }
}

} // namespace argon
