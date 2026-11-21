// Argon: a read model made into draws on the GPU, its cubes, water and animation (model_builder.hpp).
#include "render/model_builder.hpp"

#include "gfx/vulkan/vulkan_handles.hpp"
#include "logic/water_waves.hpp"
#include "render/bump_basis.hpp"
#include "render/mesh_fog.hpp"
#include "render/water_basis.hpp"
#include "track/track_source.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <span>

using argon::gfx::SamplerMode;

namespace
{

// the water shader holds slots 0..2, the only ones water.dat uses
const int kWaterCubeSlots = 3;
const size_t kMaxWaterPatches = 256;
const int kWaveFrames = 30;
const int kWaveGridSide = 17;
const int kWaveGridVertices = kWaveGridSide * kWaveGridSide;

constexpr int32_t kDiffuseSpecularBumpKind = 5;   // mesh_shading.glsl's material kind
constexpr int32_t kHudLensKind = 6;
constexpr int32_t kColorScaleKind = 7;
constexpr int32_t kSpheremapDecalKind = 8;

// the surface's D3D lighting (0xba4f0 -> 0xba370 / 0xba1d0); the bone record is 0xba4f0's third argument (0xb720f)
argon::SurfaceLighting surfaceLighting(const argon::CmmMesh &mesh)
{
    argon::SurfaceSetup setup;
    setup.hasBlock = mesh.hasSurface;
    setup.setup = mesh.surfaceSetup;
    setup.flags = mesh.surfaceFlags;
    setup.hasBoneRecord = mesh.bone >= 0 && (mesh.flags & argon::original::mesh_flags::kUsesBoneState) != 0;
    return argon::surfaceLighting(setup);
}

bool usesOwnColour(const argon::SurfaceLighting &lighting)
{
    return lighting.lit && (lighting.ambient == argon::LightSource::Own || lighting.light0 == argon::LightSource::Own);
}

// mesh_shading.glsl's material bits 32..512: lighting on, then where its ambient and light 0 come from
int32_t lightingBits(const argon::SurfaceLighting &lighting)
{
    if (!lighting.lit)
    {
        return 0;
    }
    return 32 | (lighting.ambient == argon::LightSource::Own ? 64 : 0) | (lighting.ambient == argon::LightSource::Black ? 128 : 0) |
           (lighting.light0 == argon::LightSource::Own ? 256 : 0) | (lighting.light0 == argon::LightSource::Black ? 512 : 0);
}

// the own colour (lighting block +0x00, 0xba3c3: x intensity, clamped, x tint >> 8; the tint goes in per draw)
void useOwnColour(const argon::CmmMesh &mesh, argon::GpuMaterial &material)
{
    for (int channel = 0; channel < 3; ++channel)
    {
        material.params[1 + channel] = float((mesh.surfaceColour >> (16 - 8 * channel)) & 255) / 256.f;
    }
}

constexpr float kTrackFogCurve = -1.f;   // mesh_shading.glsl: the track's fog

// the surface setup's fog (render\mesh_fog.hpp); a mesh without a surface block keeps the track's
void useMeshFog(const argon::CmmMesh &mesh, argon::GpuMaterial &material)
{
    const argon::fog::MeshFog fog =
        mesh.hasSurface ? argon::fog::meshFog(mesh.surfaceFlags, mesh.fogMode, mesh.fogColour, mesh.fogDensity, mesh.fogRange) : argon::fog::MeshFog{};
    if (fog.source == argon::fog::Source::Track)
    {
        material.fog[3] = kTrackFogCurve;
        return;
    }
    for (int channel = 0; channel < 3; ++channel)
    {
        material.fog[channel] = float((fog.colour >> (16 - 8 * channel)) & 255) / 255.f;
    }
    material.fog[3] = float(fog.curve);
    material.fogShape[0] = fog.density;
    material.fogShape[1] = fog.range;
}

// 0xb7d29: a mesh drawn with an identity world matrix (mesh flags 0x44000000) skips the render list's camera step;
// skinned meshes are drawn in model space here
argon::CameraPlacement cameraPlacement(const argon::CmmMesh &mesh)
{
    namespace sf = argon::original::surface_flags;
    argon::CameraPlacement placement;
    if (!mesh.hasSurface || mesh.skinned || (mesh.flags & argon::original::mesh_flags::kIdentityTransform))
    {
        return placement;
    }
    placement.followsCamera = (mesh.surfaceFlags & sf::kFollowsCamera) != 0;
    placement.facesCamera = (mesh.surfaceFlags & sf::kFacesCamera) != 0;
    placement.billboard = (mesh.surfaceFlags & sf::kBillboard) != 0;
    placement.billboardFactor = mesh.billboardFactor;
    return placement;
}

// the mesh's material selects the model's effect record of that name (material +0xe0)
bool usesEffect(const argon::Cmm &cmm, const argon::CmmMesh &mesh, const char *name)
{
    return mesh.effect >= 0 && size_t(mesh.effect) < cmm.effects.size() && cmm.effects[size_t(mesh.effect)] == name;
}

bool usesHudLens(const argon::Cmm &cmm, const argon::CmmMesh &mesh)
{
    return usesEffect(cmm, mesh, "Hud Lens");
}

// "Color Scale No Texture" (0x7a430 / 0x7a4b0; docs\models\effects.md "Effects"): the ships' pod glows, Kalau's, Mars's and Mount's
// lamps; oD0 = vertex colour x the bone record's tinted colour (+0x10c), no lighting, no fog, no textures
bool usesColorScale(const argon::Cmm &cmm, const argon::CmmMesh &mesh)
{
    return usesEffect(cmm, mesh, "Color Scale No Texture");
}

// "Diffuse Specular Bump Map" (0x7d5c0 / 0x7d6e0, shaders\diffusespecalpha.xpu; docs\models\effects.md "Effects") and
// "SpecAlphaHierarchy - 1 Tex" (0x7ca40 / 0x7d0e0, the same pixel shader and constants; the crafts and front-end ships):
// mesh_shading.glsl's kind 5 lights stage 0 (UV set 0) by the stage-1 normal map (UV set 1) whatever the material's combine
// codes. Its ambient (pixel shader constant 0) is the scene ambient (0x50e2f4: the track's, prop.dat, 0xb04a0; a
// craft's own, 0x59bc0) unless the mesh's setup-1 surface has
// flag 4, which makes its colour x intensity (1) x tint (255 / 256) the ambient (0xba370); params = that colour, or -1.
void useDiffuseSpecularBump(const argon::CmmMesh &mesh, argon::GpuMaterial &material)
{
    material.hdr[2] = kDiffuseSpecularBumpKind;
    const bool ownAmbient = mesh.hasSurface && mesh.surfaceSetup == 1 && (mesh.surfaceFlags & 4);
    for (int channel = 0; channel < 3; ++channel)
    {
        const uint32_t byte = (mesh.surfaceColour >> (16 - 8 * channel)) & 255;
        material.params[1 + channel] = ownAmbient ? float(byte * 255 / 256) / 255.f : -1.f;
    }
}

// the slots cmm_draw_mesh binds (mesh +0x34 / +0x38, 0xb7290..0xb7329) with the rates its material copied to them
// (0xba716); a stage without a material stage keeps the load's 60 (0xb67a0)
argon::MeshTextures meshTextures(const argon::CmmMesh &mesh, const argon::GpuMaterial &material)
{
    const float loadedRate = 60.f;
    argon::MeshTextures textures;
    textures.count = int(std::min<size_t>(mesh.tex.size(), argon::kTextureStages));
    for (int s = 0; s < textures.count; ++s)
    {
        textures.texture[size_t(s)] = mesh.tex[size_t(s)];
        textures.rate[size_t(s)] = s < material.hdr[0] ? material.stages[s].anim[0] : loadedRate;
    }
    return textures;
}

// the model's texture of that index if it loaded
const Texture *loadedTexture(const Track &model, int index)
{
    return index >= 0 && index < int(model.textures.size()) && model.textures[size_t(index)].valid ? &model.textures[size_t(index)] : nullptr;
}

// the mesh's material stages; without a material record a plain MODULATE of stage 0
std::vector<argon::CmmStage> materialStages(const argon::Cmm &cmm, const argon::CmmMesh &mesh)
{
    std::vector<argon::CmmStage> stages = cmm.stages(mesh);
    if (stages.empty() && !mesh.tex.empty())
    {
        argon::CmmStage stage;
        stage.tex = mesh.tex[0];
        stages.push_back(stage);
    }
    return stages;
}

int drawGroup(const argon::CmmMesh &mesh, bool isSky)
{
    if (isSky)
    {
        return G_SKY;
    }
    if (mesh.bone >= 0)
    {
        return G_ANIMATED;
    }
    if (!mesh.drawn)
    {
        return G_INVISIBLE;
    }
    return mesh.collidable ? G_COLLISION : G_VISUAL;
}

// 0x50670: every bone whose name has "JET" is a jet (scaled (1, 1, size)), "JFLARE" a flare (tinted)
void findJetsAndFlares(Track &model)
{
    model.jetOf.assign(model.boneName.size(), -1);
    model.flareOf.assign(model.boneName.size(), 0);
    for (size_t b = 0; b < model.boneName.size(); ++b)
    {
        for (int a = int(b), depth = 0; a >= 0 && depth < 64; a = model.boneParent[size_t(a)], ++depth)
        {
            if (model.boneName[size_t(a)].find("JFLARE") != std::string::npos)
            {
                model.flareOf[b] = 1;
                break;
            }
            if (model.boneName[size_t(a)].find("JET") != std::string::npos)
            {
                model.jetOf[b] = a;
                break;
            }
        }
    }
}

// the water.dat boxes' 128 m patches (0xcdc20 / 0xcdb60), each once
void placeWaterPatches(Track &model)
{
    for (const auto &box : model.data.water)
    {
        if (box.flags & 1)
        {
            continue;
        }
        for (float z = box.min[2]; z <= box.max[2]; z += 8.f)
        {
            for (float x = box.min[0]; x <= box.max[0]; x += 8.f)
            {
                const float px = std::floor(x * 0.0078125f) * 128.f, pz = std::floor(z * 0.0078125f) * 128.f;
                bool placed = false;
                for (const auto &patch : model.waterPatches)
                {
                    placed |= patch.pos.x == px && patch.pos.z == pz;
                }
                if (!placed && model.waterPatches.size() < kMaxWaterPatches)
                {
                    model.waterPatches.push_back({glm::vec3(px, box.max[1], pz), std::min(int(box.cubeSlot), kWaterCubeSlots - 1)});
                }
            }
        }
    }
}

}  // namespace

argon::gfx::TextureSet createStageSet(argon::gfx::Device &device, argon::gfx::SetPool pool, const StageTextures &stages)
{
    argon::gfx::TextureSetDesc desc;
    for (size_t i = 0; i < 4; ++i)
    {
        desc.textures[i] = stages.textures[i]->handle;
        desc.samplers[i] = stages.samplers[i];
    }
    return device.createTextureSet(pool, desc);
}

void ModelBuilder::create(const Setup &setup)
{
    device_ = &setup.device;
    white_ = &setup.white;
    raceTrack_ = &setup.raceTrack;
    game_ = setup.gameFolder;
    dumpMaterials_ = setup.dumpMaterials;
    std::vector<uint8_t> weaponFile;
    if (!argon::readFile(game_ + "/data/weapon.nwp", weaponFile))
    {
        weaponFile.clear();
    }
    weaponBones_ = argon::WeaponBones::fromWeaponFile(weaponFile);
}

bool ModelBuilder::load(Track &model, const std::string &folder, const ModelUse &use)
{
    ModelSource source = readModel(folder);
    return build(model, source, use);
}

bool ModelBuilder::build(Track &model, ModelSource &source, const ModelUse &use)
{
    while (model.textures.size() < source.textures.size())
    {
        uploadTexture(model, source);
    }
    return buildMeshes(model, source, use);
}

// a missing texture keeps the indices aligned, drawn white
void ModelBuilder::uploadTexture(Track &model, const ModelSource &source)
{
    const size_t size = model.textures.size();
    const argon::TextureData &picture = source.textures[size];
    if (picture.levels.empty())
    {
        model.textures.push_back(Texture{});
        return;
    }
    Texture texture = device_->createTexture(picture);
    texture.bump = source.cmm.texInfo[size].type == 3;   // detail normal map
    model.textures.push_back(texture);
}

bool ModelBuilder::buildMeshes(Track &model, ModelSource &source, const ModelUse &use)
{
    model.warnings.insert(model.warnings.end(), source.warnings.begin(), source.warnings.end());
    if (!source.loaded)
    {
        return false;
    }
    setBones(model, source.cmm);
    Build build{source, use, {}};
    for (const argon::CmmMesh &mesh : source.cmm.meshes)
    {
        addDraw(model, build, mesh);
    }
    if (!use.isSky && !use.prop)
    {   // what the game builds its collision from
        model.collTris.insert(model.collTris.end(), source.collTris.begin(), source.collTris.end());
        model.collSurf.insert(model.collSurf.end(), source.collSurf.begin(), source.collSurf.end());
    }
    if (!source.verts.empty())
    {
        uploadBuffers(model, build);
    }
    return true;
}

void ModelBuilder::release(Track &model)
{
    if (model.animVb[0])
    {
        model.vb = {};   // one of animVb[]
        for (auto &buffer : model.animVb)
        {
            device_->destroyBuffer(buffer);
        }
    }
    device_->destroyBuffer(model.vb);
    device_->destroyBuffer(model.ib);
    device_->destroyBuffer(model.lines);
    device_->destroyBuffer(model.matBuf);
    device_->destroyBuffer(model.waterVb);
    device_->destroyBuffer(model.waterIb);
    device_->destroyBuffer(model.waterWave);
    for (Texture &cubeTexture : model.envCubes)
    {
        device_->destroyTexture(cubeTexture);
    }
    device_->destroyTexture(model.waterBump);
    for (auto &texture : model.textures)
    {
        device_->destroyTexture(texture);
    }
    model = Track{};
}

// ------------------------------------------------------------------ cubes

// the track's environment cube slot 0..7 (argon::kEnvCubeSlots; data\defaultcubemap as the game's fallback) as a
// 6-layer 2D array: the shaders do the cube face lookup themselves (texture set 0 only holds 2D arrays)
const Texture *ModelBuilder::cube(Track &track, int slot)
{
    slot = std::clamp(slot, 0, argon::kEnvCubeSlots - 1);
    if (!track.cubeTried[size_t(slot)])
    {
        setCube(track, slot, readTrackCube(game_, track.name, slot));
    }
    const Texture &texture = track.envCubes[size_t(slot)];
    return texture.valid ? &texture : white_;
}

void ModelBuilder::setCube(Track &track, int slot, const argon::TextureData &env)
{
    track.cubeTried[size_t(slot)] = true;
    if (!env.levels.empty())
    {
        track.envCubes[size_t(slot)] = device_->createTexture(env);
    }
}

// the specular-bump effects bind the one global cube table (the race track's envdata): a pickup or ship takes the
// loaded track's (the mega-up's core reflects it); with no track loaded (the front end) its own folder's
Track &ModelBuilder::cubeOwner(Track &model)
{
    return raceTrack_->name.empty() ? model : *raceTrack_;
}

// 0: the model's own cubes, else the race track's
uint64_t ModelBuilder::cubeOwnerKey(Track &model, uint64_t raceSerial)
{
    return &cubeOwner(model) == &model ? 0 : raceSerial;
}

// the draw about to be added to model.draws takes an environment cube
void ModelBuilder::registerCubeDraw(Track &model, const MaterialBinding &binding, argon::gfx::SetPool pool)
{
    Track::CubeDraws &use = model.cubeDraws[binding.key];
    for (size_t i = 0; i < 4; ++i)
    {
        use.desc.textures[i] = binding.stages.textures[i]->handle;
        use.desc.samplers[i] = binding.stages.samplers[i];
    }
    use.slot = binding.cubeSlot;
    use.pool = pool;
    use.draws.push_back(uint32_t(model.draws.size()));
    model.cubeOwner = ~uint64_t(0);
}

void ModelBuilder::refreshCubes(Track &model, argon::gfx::SetPool racePool, uint64_t raceSerial)
{
    const uint64_t owner = cubeOwnerKey(model, raceSerial);
    if (model.cubeDraws.empty() || model.cubeOwner == owner)
    {
        return;
    }
    model.cubeOwner = owner;
    Track &cubes = cubeOwner(model);
    for (auto &[key, use] : model.cubeDraws)
    {
        use.desc.textures[1] = cube(cubes, use.slot)->handle;
        argon::gfx::TextureSet set;
        if (owner != 0)
        {
            set = device_->createTextureSet(racePool, use.desc);
        }
        else
        {
            if (!use.ownSet)
            {
                use.ownSet = device_->createTextureSet(use.pool, use.desc);
            }
            set = use.ownSet;
        }
        for (const uint32_t draw : use.draws)
        {
            model.draws[draw].texSet = set;
        }
    }
}

// ------------------------------------------------------------------ water

void ModelBuilder::buildWater(Track &track, const argon::TextureData &bump, argon::gfx::SetPool pool)
{
    placeWaterPatches(track);
    if (track.waterPatches.empty())
    {
        return;
    }
    uploadWaterWaves(track);
    track.waterWaveSet = device_->createMaterialSet(pool, track.waterWave);
    uploadWaterGrid(track);

    // textures: bump frames (stage 0), cube slots 0..2 as 2D arrays (manual face lookup in the shader)
    if (!bump.levels.empty())
    {
        track.waterBump = device_->createTexture(bump);
    }
    else
    {
        track.warnings.push_back("water: data\\testdata\\waterbump000..059.dds missing");
    }
    StageTextures stages;
    stages.textures = {track.waterBump.valid ? &track.waterBump : white_, cube(track, 0), cube(track, 1), cube(track, 2)};
    stages.samplers = {SamplerMode::Wrap, SamplerMode::Clamp, SamplerMode::Clamp, SamplerMode::Clamp};
    track.waterSet = createStageSet(*device_, pool, stages);
}

// wave frames (argon::WaterWaves, 0xcd4f0), stored as NORMSHORT (x c4.y = water.dat amplitude); 8 m grid steps
void ModelBuilder::uploadWaterWaves(Track &track)
{
    const int side = kWaveGridSide;
    const float amp = track.data.waterAmp;
    const argon::WaterWaves waves(amp);
    std::vector<glm::vec4> wave(size_t(kWaveFrames) * kWaveGridVertices * 4);
    for (int f = 0; f < kWaveFrames; ++f)
    {
        const std::vector<argon::BumpBasis> basis = argon::buildWaterBasis(waves, f);
        int h[kWaveGridSide][kWaveGridSide];
        for (int j = 0; j < side; ++j)
        {
            for (int i = 0; i < side; ++i)
            {
                h[j][i] = waves.height(f, i, j);
            }
        }
        for (int j = 0; j < side; ++j)
        {
            for (int i = 0; i < side; ++i)
            {
                // the Fresnel normal (stream 2, 0xcd4f0): central differences with the game's wrap (i-1 < 0 -> 16)
                const int d1 = h[j][(i + 1) % side] - h[j][i > 0 ? i - 1 : side - 1];
                const int d2 = h[(j + 1) % side][i] - h[j > 0 ? j - 1 : side - 1][i];
                const glm::vec3 ta = glm::normalize(glm::vec3(16.f, d1 * amp / 32768.f, 0.f));
                const glm::vec3 tb = glm::normalize(glm::vec3(0.f, d2 * amp / 32768.f, 16.f));
                const glm::vec3 normal = -glm::normalize(glm::cross(ta, tb));
                // the bump basis (v1..v3, 0xcd170: render/water_basis.cpp); the normal rides in the w slots
                const argon::BumpBasis &vertexBasis = basis[size_t(j * side + i)];
                const size_t offset = (size_t(f) * kWaveGridVertices + j * side + i) * 4;
                wave[offset + 0] = glm::vec4(8.f * i, h[j][i] / 32767.f * amp, 8.f * j, normal.x);
                wave[offset + 1] = glm::vec4(vertexBasis.tangent, normal.y);
                wave[offset + 2] = glm::vec4(vertexBasis.binormal, normal.z);
                wave[offset + 3] = glm::vec4(vertexBasis.normal, 0.f);
            }
        }
    }
    track.waterWave = device_->uploadBuffer(argon::gfx::BufferUse::Storage, std::as_bytes(std::span(wave)));
}

// the 17 x 17 grid (the shader reads the wave frames by vertex index)
void ModelBuilder::uploadWaterGrid(Track &track)
{
    const int side = kWaveGridSide;
    std::vector<argon::Vertex> vertices(kWaveGridVertices);
    for (int k = 0; k < kWaveGridVertices; ++k)
    {
        vertices[size_t(k)].pos[0] = float(k % side);
        vertices[size_t(k)].pos[2] = float(k / side);
    }
    std::vector<uint32_t> indices;
    for (int j = 0; j < side - 1; ++j)
    {
        for (int i = 0; i < side - 1; ++i)
        {
            const uint32_t corner = j * side + i;
            indices.insert(indices.end(), {corner, corner + 1, corner + side, corner + 1, corner + side + 1, corner + side});
        }
    }
    track.waterVb = device_->uploadBuffer(argon::gfx::BufferUse::Vertex, std::as_bytes(std::span(vertices)));
    track.waterIb = device_->uploadBuffer(argon::gfx::BufferUse::Index, std::as_bytes(std::span(indices)));
    track.waterIndexCount = uint32_t(indices.size());
}

// ------------------------------------------------------------------ draws

void ModelBuilder::setBones(Track &model, const argon::Cmm &cmm)
{
    model.boneName.clear();
    model.boneParent.clear();
    model.boneRest.clear();
    model.boneRestInverse.clear();
    for (size_t b = 0; b < cmm.bones.size(); ++b)
    {
        model.boneName.push_back(cmm.bones[b].name);
        model.boneParent.push_back(cmm.bones[b].parent);
        glm::mat4 world(1.f);
        cmm.boneWorld(int(b), glm::value_ptr(world));
        model.boneRest.push_back(world);
        model.boneRestInverse.push_back(glm::inverse(world));
    }
    model.flaps = argon::FlapRig(model.name, cmm);
    findJetsAndFlares(model);
}

void ModelBuilder::addDraw(Track &model, Build &build, const argon::CmmMesh &mesh)
{
    const argon::Cmm &cmm = build.source.cmm;
    const MeshRange &range = build.source.meshes[size_t(&mesh - cmm.meshes.data())];
    if (range.indexCount == 0)
    {
        return;
    }
    Draw draw;
    draw.mesh = mesh.index;
    draw.firstIndex = range.firstIndex;
    draw.indexCount = range.indexCount;
    draw.vertexOffset = range.vertexOffset;
    draw.tex = !mesh.tex.empty() && loadedTexture(model, mesh.tex[0]) ? mesh.tex[0] : -1;
    draw.st = argon::decodeMeshState(mesh.hasSurface, mesh.blendDesc, mesh.surfaceFlags);
    draw.hudLens = usesHudLens(cmm, mesh);
    draw.colorScale = usesColorScale(cmm, mesh);
    draw.lighting = surfaceLighting(mesh);
    draw.tintLit = usesOwnColour(draw.lighting);
    if (draw.hudLens)
    {
        draw.st.blend = false;   // 0x7a803: the effect turns blending off (NV2A 0x304 = 0): the star replaces its pixels
    }
    bindMaterial(model, build, mesh, draw);
    if (draw.st.blend)
    {
        model.blendedCount++;
    }
    draw.group = drawGroup(mesh, build.use.isSky);
    if (mesh.bone >= 0 && !mesh.skinned)
    {
        cmm.boneWorld(mesh.bone, glm::value_ptr(draw.model));   // skinned: model space
    }
    draw.sortByPoint = (mesh.flags & argon::original::mesh_flags::kSortPoint) != 0;
    draw.sortPoint = glm::vec3(mesh.sortPoint[0], mesh.sortPoint[1], mesh.sortPoint[2]);
    draw.occlTest = mesh.hasSurface && (mesh.surfaceFlags & 0x40000) && mesh.occlPoints > 0;
    draw.occlRadius = mesh.occlRadius;
    draw.occlPoints = int(std::min<uint32_t>(mesh.occlPoints, 256));
    draw.occlCopy = mesh.hasSurface && (mesh.surfaceFlags & 0x80000) && mesh.bone >= 0;
    draw.camera = cameraPlacement(mesh);
    draw.shadowHull = build.use.craft && mesh.hasSurface && (mesh.surfaceFlags & argon::original::surface_flags::kShadowHull);
    model.hasShadowHull |= draw.shadowHull;
    draw.bmin = range.placedMin;
    draw.bmax = range.placedMax;
    draw.bone = mesh.bone;
    if (build.use.prop && build.use.weaponParts && mesh.bone >= 0)
    {
        draw.fx = weaponBones_.marks(cmm.bones, mesh.bone);
    }
    if (draw.group == G_COLLISION || draw.group == G_VISUAL)
    {
        model.bmin = glm::min(model.bmin, range.localMin);
        model.bmax = glm::max(model.bmax, range.localMax);
    }
    model.groupCount[draw.group]++;
    model.triangles += range.indexCount / 3;
    model.draws.push_back(draw);
}

// the draw's material, and its texture set: shared by key, or (a specular-bump effect) made by refreshCubes
void ModelBuilder::bindMaterial(Track &model, Build &build, const argon::CmmMesh &mesh, Draw &draw)
{
    const Material material = buildMaterial(model, MeshBuild{build.source.cmm, mesh, draw, build.use.isSky});
    build.materials.push_back(material.gpu);
    draw.matIndex = uint32_t(build.materials.size() - 1);
    draw.textures = meshTextures(mesh, material.gpu);
    const MaterialBinding &binding = material.binding;
    if (binding.cubeSlot >= 0)
    {
        registerCubeDraw(model, binding, build.use.pool);
        return;
    }
    auto known = model.texSets.find(binding.key);
    draw.texSet = known != model.texSets.end() ? known->second : (model.texSets[binding.key] = createStageSet(*device_, build.use.pool, binding.stages));
}

// GpuMaterial for one mesh: its texture stages (combine ops, uv set, scroll, animation), and the 4 textures + sampler
// modes it binds, packed into a texture set key
ModelBuilder::Material ModelBuilder::buildMaterial(Track &model, const MeshBuild &mesh)
{
    const argon::CmmMesh &cmmMesh = mesh.mesh;
    const Draw &draw = mesh.draw;
    const std::vector<argon::CmmStage> stages = materialStages(mesh.cmm, cmmMesh);
    Material material;
    argon::GpuMaterial &gpuMaterial = material.gpu;
    const bool additive = draw.st.blend && draw.st.dst == argon::gfx::BlendFactor::One;
    const Texture *t0 = stages.empty() ? nullptr : loadedTexture(model, stages[0].tex);
    gpuMaterial.hdr[0] = int32_t(stages.size());
    gpuMaterial.hdr[1] = 1;   // every vertex type carries a D3DCOLOR (prelit)
    gpuMaterial.hdr[2] = mesh.isSky ? 1 : additive ? 2 : 0;
    gpuMaterial.hdr[3] = (t0 && t0->cutout ? 1 : 0) | (t0 && t0->alpha ? 2 : 0) | (draw.st.a2c ? 4 : 0) |   // 4 = alpha-to-coverage
               (cmmMesh.skinned ? 8 : 0) |                                                     // 8 = skinned (0xa3df0: all stages MODULATE when the light is black)
               (cmmMesh.hasSurface && (cmmMesh.surfaceFlags & 0x2000) ? 16 : 0) |                    // 16 = exempt from 0xbad5b's reset
               lightingBits(draw.lighting);
    gpuMaterial.params[0] = draw.st.translucentPass ? std::max(draw.st.alphaRef, model.translucentAlphaRef) : draw.st.alphaRef;
    gpuMaterial.params[1] = gpuMaterial.params[2] = gpuMaterial.params[3] = 1.f;
    if (usesOwnColour(draw.lighting))
    {
        useOwnColour(cmmMesh, gpuMaterial);
    }
    useMeshFog(cmmMesh, gpuMaterial);
    MaterialBinding &binding = material.binding;
    bool anim = false, env = false;
    for (int i = 0; i < 4; ++i)
    {
        binding.stages.textures[size_t(i)] = white_;
        binding.stages.samplers[size_t(i)] = SamplerMode::Wrap;
        if (i >= int(stages.size()))
        {
            continue;
        }
        const argon::CmmStage &stage = stages[size_t(i)];
        const Texture *loaded = loadedTexture(model, stage.tex);
        const Texture *tx = loaded ? loaded : white_;
        const int mode = ((stage.flags & 0x40) ? 1 : 0) | ((stage.flags & 0x80) ? 2 : 0);
        binding.stages.textures[size_t(i)] = tx;
        binding.stages.samplers[size_t(i)] = SamplerMode(mode);
        auto &gs = gpuMaterial.stages[i];
        gs.op[0] = argon::stageOp(stage.colorCode, argon::StageChannel::Colour);
        gs.op[1] = argon::stageOp(stage.alphaCode, argon::StageChannel::Alpha);
        gs.op[2] = int32_t(stage.flags);
        gs.op[3] = int32_t(stage.texcoord);
        gs.anim[0] = stage.rate;   // 0xba716 copies the stage's rate as it is: 0 keeps an animated texture on frame 0
        gs.anim[1] = float(tx->layers);
        gs.anim[2] = stage.scrollU * 60.f;   // game: speed * 60 * time (FLOAT_001a346c)
        gs.anim[3] = stage.scrollV * 60.f;
        gs.extra[0] = stage.offU;
        gs.extra[1] = stage.offV;
        gs.extra[2] = tx->bump ? 1.f : 0.f;
        anim |= tx->layers > 1 || (stage.flags & 1);
        env |= (stage.flags & 8) != 0;
        const uint64_t slot = uint64_t((tx == white_ ? 0 : stage.tex + 1) & 0x7ff) | (uint64_t(mode) << 11);
        binding.key |= slot << (13 * i);
    }
    applyEffect(model, mesh, stages.size(), material);
    model.multiStage += stages.size() > 1;
    model.animated += anim;
    model.envMapped += env;
    if (dumpMaterials_ && !mesh.isSky)
    {
        dumpMaterial(model, mesh, stages);
    }
    return material;
}

// effect records (material +0xe0): "Specular Bump" (0x7ba90 / 0x7bb80, pixel shader 0x2907f0: rgb =
// reflection, alpha = bump alpha) and "Specular Bump RGBA" (0x7bd50 / 0x7be50, pixel shader 0x2907ec:
// rgb = reflection x vertex colour, alpha = vertex alpha): the stage-0 texture is a normal map that bends
// a cube-map reflection (texm3x3vspec), cube = mesh +0xa8. Bracken's FLATwater / popwater use them.
void ModelBuilder::applyEffect(Track &model, const MeshBuild &mesh, size_t stageCount, Material &material)
{
    const argon::Cmm &cmm = mesh.cmm;
    const argon::CmmMesh &cmmMesh = mesh.mesh;
    argon::GpuMaterial &gpuMaterial = material.gpu;
    int fx = 0;
    if (cmmMesh.effect >= 0 && cmmMesh.effect < int(cmm.effects.size()))
    {
        if (cmm.effects[size_t(cmmMesh.effect)] == "Specular Bump")
        {
            fx = 3;
        }
        else if (cmm.effects[size_t(cmmMesh.effect)] == "Specular Bump RGBA")
        {
            fx = 4;
        }
        else if (argon::usesDiffuseSpecularBump(cmm, cmmMesh) && stageCount >= 2 && !mesh.isSky)
        {
            fx = kDiffuseSpecularBumpKind;
        }
    }
    if (usesHudLens(cmm, cmmMesh) && !mesh.isSky)
    {
        gpuMaterial.hdr[2] = kHudLensKind;   // the mega-up's glass star (0x7a600 / 0x7a6a0)
    }
    else if (usesColorScale(cmm, cmmMesh) && !mesh.isSky)
    {
        gpuMaterial.hdr[2] = kColorScaleKind;
    }
    else if (usesEffect(cmm, cmmMesh, "Spheremap Decal") && !mesh.isSky && stageCount >= 2)
    {   // Kutoo's and Jaya's front-end heads (0x7a850 / 0x7a920): stage 1 reads the program's sphere-map oT1
        gpuMaterial.hdr[2] = kSpheremapDecalKind;
    }
    else if (fx == kDiffuseSpecularBumpKind)
    {
        useDiffuseSpecularBump(cmmMesh, gpuMaterial);
    }
    else if (fx && !mesh.isSky && stageCount > 0)
    {
        MaterialBinding &binding = material.binding;
        gpuMaterial.hdr[2] = fx;
        gpuMaterial.stages[0].op[2] |= 1;   // the effect always applies stage 0's scroll and offset (material +0x28..+0x34)
        binding.stages.textures[1] = cube(cubeOwner(model), cmmMesh.cubeIndex);
        binding.stages.samplers[1] = SamplerMode::Clamp;
        binding.cubeSlot = cmmMesh.cubeIndex;
        binding.key = (binding.key & ~(uint64_t(0x1fff) << 13)) | (uint64_t((0x7f8 + (cmmMesh.cubeIndex & 7)) | (3u << 11)) << 13);
    }
}

// --dump-materials: one line per mesh with its stages
void ModelBuilder::dumpMaterial(const Track &model, const MeshBuild &mesh, const std::vector<argon::CmmStage> &stages) const
{
    const argon::Cmm &cmm = mesh.cmm;
    const argon::CmmMesh &cmmMesh = mesh.mesh;
    const bool hasEffect = cmmMesh.effect >= 0 && cmmMesh.effect < int(cmm.effects.size());
    std::printf("mesh %3d vtype %d stages %d%s%s:",
                int(&cmmMesh - cmm.meshes.data()),
                cmmMesh.vtype,
                int(stages.size()),
                hasEffect ? " fx " : "",
                hasEffect ? cmm.effects[size_t(cmmMesh.effect)].c_str() : "");
    for (const auto &stage : stages)
    {
        const Texture *tx = (stage.tex >= 0 && stage.tex < int(model.textures.size())) ? &model.textures[size_t(stage.tex)] : nullptr;
        std::printf("  [tex %d%s uv%d fl %#x col %d alp %d scroll %.3g,%.3g rate %.3g]",
                    stage.tex,
                    tx && tx->bump         ? " bump"
                    : tx && tx->layers > 1 ? " anim"
                                           : "",
                    stage.texcoord,
                    stage.flags,
                    stage.colorCode,
                    stage.alphaCode,
                    stage.scrollU,
                    stage.scrollV,
                    stage.rate);
    }
    std::printf("\n");
}

// animated models (bones posed every frame) keep the cmm; the re-skinned ones (ModelUse::reskinned) host-visible
// vertex buffers as well, the rest GPU memory
void ModelBuilder::uploadBuffers(Track &model, Build &build)
{
    const bool posed = build.source.cmm.animated() || (model.posable && !build.source.cmm.bones.empty());
    if (posed && build.use.reskinned)
    {
        uploadReskinnedVertices(model, build.source.verts);
    }
    else
    {
        model.vb = device_->uploadBuffer(argon::gfx::BufferUse::Vertex, std::as_bytes(std::span(build.source.verts)));
    }
    if (posed)
    {
        model.anim = std::make_shared<argon::Cmm>(std::move(build.source.cmm));
    }
    model.ib = device_->uploadBuffer(argon::gfx::BufferUse::Index, std::as_bytes(std::span(build.source.indices)));
    model.matBuf = device_->uploadBuffer(argon::gfx::BufferUse::Storage, std::as_bytes(std::span(build.materials)));
    model.matSet = device_->createMaterialSet(build.use.pool, model.matBuf);
}

// one host-visible vertex buffer per frame in flight, rewritten by skinPosed, and the vertices it skins from
void ModelBuilder::uploadReskinnedVertices(Track &model, const std::vector<argon::Vertex> &vertices)
{
    const size_t bytes = vertices.size() * sizeof(argon::Vertex);
    for (size_t f = 0; f < std::size(model.animVb); ++f)
    {
        model.animVb[f] = device_->createStreamBuffer(argon::gfx::BufferUse::Vertex, bytes);
        model.animMapped[f] = device_->mapped(model.animVb[f]);
        std::memcpy(model.animMapped[f], vertices.data(), bytes);
    }
    model.vb = model.animVb[0];
    model.animVerts = vertices;
}

// ------------------------------------------------------------------ animation

// the character pages set the clock to the seconds since the character first showed (0x15db0 -> +0x5e4); morph
// meshes blend their keyframes (0x7b7e0) at the same clock (the game's is the effect's +0x3c, not traced)
void ModelBuilder::animate(Track &model, ModelClock clock)
{
    if (!model.anim)
    {
        return;
    }
    model.anim->pose(clock.seconds, model.animWorld);
    skinPosed(model, clock);
}

void ModelBuilder::animateWith(Track &model, const argon::PoseDrivers &drivers, ModelClock clock)
{
    if (!model.anim)
    {
        return;
    }
    model.anim->pose(drivers.keys, drivers.aims, model.animWorld);
    skinPosed(model, clock);
}

// the posed bones (model.animWorld) into this frame's vertex buffer
void ModelBuilder::skinPosed(Track &model, ModelClock clock)
{
    const argon::Cmm &cmm = *model.anim;
    for (auto &draw : model.draws)
    {
        if (draw.mesh < 0 || draw.mesh >= int(cmm.meshes.size()))
        {
            continue;
        }
        const argon::CmmMesh &mesh = cmm.meshes[size_t(draw.mesh)];
        const bool verticesFit = size_t(draw.vertexOffset) + mesh.numVerts <= model.animVerts.size();
        if (mesh.morph.streams > 0 && verticesFit)
        {
            cmm.morph(mesh, clock.seconds, &model.animVerts[size_t(draw.vertexOffset)]);
        }
        if (mesh.skinned)
        {
            if (verticesFit)
            {
                cmm.skin(mesh, model.animWorld.data(), &model.animVerts[size_t(draw.vertexOffset)]);
            }
        }
        else if (mesh.bone >= 0 && size_t(mesh.bone) * 16 + 16 <= model.animWorld.size())
        {
            draw.model = glm::make_mat4(&model.animWorld[size_t(mesh.bone) * 16]);
        }
    }
    if (!model.animVb[clock.frame])
    {
        return;   // not ModelUse::reskinned: its bone-riding meshes move, its vertices stay
    }
    std::memcpy(model.animMapped[clock.frame], model.animVerts.data(), model.animVerts.size() * sizeof(argon::Vertex));
    model.vb = model.animVb[clock.frame];
}
