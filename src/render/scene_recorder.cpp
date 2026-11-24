#include "render/scene_recorder.hpp"
#include "render/effect_passes.hpp"
#include "render/depth_lift.hpp"
#include "render/flare_occlusion.hpp"
#include "movie/story_scene.hpp"
#include "app/weapons.hpp"
#include "logic/drive_camera.hpp"
#include "logic/game_logic.hpp"
#include "logic/race.hpp"
#include "logic/race_eggs.hpp"
#include "logic/start_grid.hpp"
#include "render/frame_copies.hpp"
#include "render/model_builder.hpp"
#include "render/prop_models.hpp"
#include "render/effect_buffers.hpp"
#include "render/flare_queries.hpp"
#include "render/mesh_draw_records.hpp"
#include "render/order_independent_translucency.hpp"
#include "render/scene_buffers.hpp"
#include "render/ship_shadows.hpp"
#include "render/scene_toggles.hpp"
#include "render/projection.hpp"

#include <glm/gtc/type_ptr.hpp>

namespace
{
const argon::MeshState kOpaqueState;
const float kMinSortKey = 0.1f;

bool hasJetData(const Track &model, int bone, float jetSize)
{
    return jetSize >= 0.f && bone >= 0 && bone < int(model.jetOf.size());
}

// beams (0x68620): a look-at frame between the beam's start and end, at the start, z scaled by the length (the beam
// bone's mesh spans z 0..1)
glm::mat4 beamMatrix(const argon::weapons::Shot &shot)
{
    const glm::vec3 along = shot.beamEnd - shot.pos;
    const float length = glm::length(along);
    if (length < 1e-3f)
    {
        return glm::mat4(0.f);
    }
    const glm::vec3 z = along / length;
    const glm::vec3 side = glm::cross(glm::vec3(0, 1, 0), z);
    const glm::vec3 x = glm::length(side) > 1e-4f ? glm::normalize(side) : glm::vec3(1, 0, 0);
    const glm::vec3 y = glm::cross(z, x);
    return glm::mat4(glm::vec4(x, 0.f), glm::vec4(y, 0.f), glm::vec4(z * length, 0.f), glm::vec4(shot.pos, 1.f));
}

bool isBeam(const argon::weapons::Shot &shot)
{
    return shot.weaponType->cls == 1;
}

glm::mat4 shotMatrix(const argon::weapons::Shot &shot)
{
    if (isBeam(shot))
    {
        return beamMatrix(shot);
    }
    return glm::mat4(glm::vec4(shot.right, 0.f), glm::vec4(shot.up, 0.f), glm::vec4(shot.fwd, 0.f), glm::vec4(shot.pos, 1.f));
}

// a bone tint (ARGB, 0xb9930 -> +0x104) as the shader takes it: RGB / 256
glm::vec3 boneTintOf(uint32_t argb)
{
    return glm::vec3(float((argb >> 16) & 0xffu), float((argb >> 8) & 0xffu), float(argb & 0xffu)) / 256.f;
}

// "Color Scale No Texture" (0x7a4b0): c4 = the bone record's tinted colour (+0x10c) / 255, made of the bone tint
// (+0x104, white at load) x the occlusion intensity (every mesh with the effect is occlusion-tested)
glm::vec3 colorScaleTint(const std::optional<glm::vec3> &boneTint, float occlusion)
{
    const glm::vec3 tintBytes = boneTint ? glm::min(glm::round(*boneTint * 256.f), glm::vec3(255.f)) : glm::vec3(255.f);
    glm::vec3 scaled;
    for (int channel = 0; channel < 3; ++channel)
    {
        scaled[channel] = argon::flare::tintedChannel(uint8_t(tintBytes[channel]), occlusion);
    }
    return scaled;
}
} // namespace

ModelPlacement ModelPlacement::forSky()
{
    ModelPlacement placement;
    placement.isSky = true;
    return placement;
}

ModelPlacement ModelPlacement::forProp(const glm::mat4 &instance)
{
    ModelPlacement placement;
    placement.instance = instance;
    placement.isProp = true;
    return placement;
}

ModelPlacement ModelPlacement::forBone(int bone, const glm::mat4 &boneMatrix)
{
    ModelPlacement placement;
    placement.isProp = true;
    placement.onlyBone = bone;
    placement.boneMatrix = boneMatrix;
    return placement;
}

SceneRecorder::SceneRecorder(SceneHost &host, SceneRaceView &race, const SceneSources &sources, argon::gfx::CommandList &commands)
    : host_(host), race_(race), sources_(sources), commands_(commands)
{
}

void SceneRecorder::setDepthCull(const argon::gfx::DepthCull &state) const
{
    commands_.setDepthCull(argon::gfx::DepthCull{state.test, state.test && state.write, state.cullBack});
}

void SceneRecorder::pushMatrices(const glm::mat4 &mvp, const glm::mat4 &model) const
{
    const PushConstants constants{mvp, model};
    commands_.pushConstants(std::as_bytes(std::span<const PushConstants, 1>(&constants, 1)));
}

// a split screen's view: the viewport and the scissor on its part of the screen
void SceneRecorder::setViewport(const argon::gfx::Rect &area) const
{
    commands_.setViewport(area);
    commands_.setFrontFace(host_.sceneFrontFace());
}

void SceneRecorder::bindVertexBuffer(argon::gfx::BufferRef buffer) const
{
    commands_.bindVertexBuffer(buffer);
}

// ------------------------------------------------------------------ one model
// pass 0: everything not in the translucent pass (file order), pass 1: translucent, matching the game's FUN_000b6e60.
// With "Game materials" off everything is opaque in pass 0. The game's draws in between bind or empty every texture
// stage, so a model's draw starts with none bound.
void SceneRecorder::drawModel(Track &model, int pass, const ModelPlacement &placement)
{
    if (!model.vb)
    {
        return;
    }
    sources_.models.refreshCubes(model, sources_.trackPool, sources_.trackSerial);
    bindCache_.emptyAll();
    bindModelBuffers(model);
    boundTextures_ = {};
    boundPipe_ = {};
    if (sortsTranslucentDraws(model, pass, placement))
    {
        sortTranslucentDraws(model);
        for (const SortedDraw &sorted : sortedDraws_)
        {
            drawMesh(MeshCall{model, model.draws[sorted.index], sorted.index, placement}, pass);
        }
        return;
    }
    for (uint32_t index = 0; index < uint32_t(model.draws.size()); ++index)
    {
        drawMesh(MeshCall{model, model.draws[index], index, placement}, pass);
    }
}

void SceneRecorder::drawModelBothPasses(Track &model, const ModelPlacement &placement)
{
    drawModel(model, 0, placement);
    drawModel(model, 1, placement);
}

void SceneRecorder::bindModelBuffers(Track &model) const
{
    commands_.bindMaterials(model.matSet);
    bindVertexBuffer(model.vb);
    commands_.bindIndexBuffer(model.ib);
}

bool SceneRecorder::sortsTranslucentDraws(const Track &model, int pass, const ModelPlacement &placement) const
{
    return &model == &sources_.track && pass == 1 && !placement.isSky && placement.onlyBone < 0 && sources_.toggles.gameMaterials;
}

namespace
{
bool drawsFartherFirst(const SceneRecorder::SortedDraw &first, const SceneRecorder::SortedDraw &second)
{
    return first.key > second.key;
}
}  // namespace

// the track's translucent pass in the race's order (0xafcf0 -> render list bucket 4, 0xbb820): far to near; equal keys
// in reverse file order (a new entry goes before the equal ones). Only the translucent draws are listed: the stable
// sort keeps their order the same as among all of them
void SceneRecorder::sortTranslucentDraws(const Track &model)
{
    sortedDraws_.clear();
    for (uint32_t index = uint32_t(model.draws.size()); index-- > 0;)
    {
        const Draw &draw = model.draws[index];
        if (meshState(draw).translucentPass)
        {
            sortedDraws_.push_back({translucentSortKey(draw), index});
        }
    }
    std::stable_sort(sortedDraws_.begin(), sortedDraws_.end(), drawsFartherFirst);
}

// the eye's distance to the box's farthest edge in x and in z (the box = the mesh's vertex bounds, 0xb9270), the
// larger of the two. Meshes with flag 0x80 are keyed by the eye's distance to their sort point (mesh +0x4c), placed by
// the artists: Silver's channel beds (460, 325) at y -27000, the speckle layers (462) at -14000, the water (73) at
// -212, so the water is always drawn over its bed
float SceneRecorder::translucentSortKey(const Draw &draw) const
{
    if (draw.st.laidOver)
    {
        return 0.f;   // 0xb02d1: surface flag 0x20000 inserts with key 0, after every other mesh (theirs are at least 0.1)
    }
    const glm::vec3 eye = sources_.camera.eye;
    if (draw.sortByPoint)
    {
        return std::max(glm::length(eye - draw.sortPoint), kMinSortKey);
    }
    const float kx = std::max({0.f, eye.x - draw.bmin.x, draw.bmax.x - eye.x});
    const float kz = std::max({0.f, eye.z - draw.bmin.z, draw.bmax.z - eye.z});
    return std::max(std::max(kx, kz), kMinSortKey);
}

bool SceneRecorder::isMeshHidden(const MeshCall &mesh) const
{
    const Draw &draw = mesh.draw;
    const ModelPlacement &pl = mesh.placement;
    if (pl.onlyBone >= 0)
    {
        return draw.bone < 0 || !mesh.model.underBone(draw.bone, pl.onlyBone);
    }
    if (pl.isProp)
    {
        return draw.fx || draw.shadowHull;
    }
    if (&mesh.model == &sources_.track && hiddenTrackBone_ >= 0 && draw.bone >= 0 && mesh.model.underBone(draw.bone, hiddenTrackBone_))
    {
        return true;
    }
    if (&mesh.model == &sources_.track && visibleTrackMeshes_ && size_t(draw.mesh) < visibleTrackMeshes_->size() && !(*visibleTrackMeshes_)[size_t(draw.mesh)])
    {
        return true;
    }
    if (!sources_.toggles.groups[draw.group])
    {
        return true;
    }
    const auto &hidden = sources_.hiddenMeshes;
    return &mesh.model == &sources_.track && std::find(hidden.begin(), hidden.end(), draw.mesh) != hidden.end();
}

const argon::MeshState &SceneRecorder::meshState(const Draw &draw) const
{
    return sources_.toggles.gameMaterials ? draw.st : kOpaqueState;
}

bool SceneRecorder::cullsBackFaces(const argon::MeshState &st) const
{
    return sources_.toggles.gameMaterials ? (sources_.toggles.gameCull && !st.twoSided) : sources_.toggles.cull;
}

// into the OIT targets: their program, the depth tested and never written. The rest of the translucent pass under
// order independence writes no depth either, so a glass mesh (Mars's ONE / SRC_ALPHA dome) cannot hide the
// translucent draws after it (docs\argon\differences.md "Translucency"). The solid parts always write it, also for a
// mesh the game draws without depth writes: the OIT composite goes over every solid pixel, so only depth keeps an
// earlier draw's translucent pixels under it (Silver's speckle layer 462 under its river 165)
void SceneRecorder::bindMeshState(const MeshCall &mesh)
{
    const argon::MeshState &st = meshState(mesh.draw);
    const bool writesDepth = translucentDraws_ == TranslucentDraws::SolidParts ||
                             (st.depthWrite && (translucentDraws_ == TranslucentDraws::All || translucentDraws_ == TranslucentDraws::OitDepth));
    const argon::gfx::Pipeline pipe = pipelineOf(st);
    if (pipe != boundPipe_)
    {
        commands_.bindPipeline(pipe);
        boundPipe_ = pipe;
    }
    const bool depth = !mesh.placement.isSky;
    setDepthCull({.test = depth && st.depthTest, .write = depth && writesDepth, .cullBack = cullsBackFaces(st)});
}

// the mesh program of the stage the translucent pass is in (the game's for every other draw)
argon::gfx::Pipeline SceneRecorder::pipelineOf(const argon::MeshState &st) const
{
    if (translucentDraws_ == TranslucentDraws::SolidParts)
    {
        return sources_.translucency.solidPipeline();
    }
    if (translucentDraws_ == TranslucentDraws::OrderIndependent)
    {
        return sources_.translucency.meshPipeline();
    }
    if (translucentDraws_ == TranslucentDraws::OitDepth)
    {
        return sources_.translucency.depthPipeline();
    }
    const argon::gfx::Fill fill =
        sources_.toggles.wireframe && sources_.device.wireframeSupported() ? argon::gfx::Fill::Wireframe : argon::gfx::Fill::Solid;
    return sources_.device.meshPipeline(st, fill);
}

// pass 0 the opaque draws, 1 the translucent ones; with order independence the translucent pass is drawn four times:
// the alpha-blended draws' solid pixels, their other pixels, their depth, then the rest
bool SceneRecorder::drawsInPass(const Draw &draw, int pass) const
{
    if (int(meshState(draw).translucentPass) != pass)
    {
        return false;
    }
    if (pass == 0 || translucentDraws_ == TranslucentDraws::All)
    {
        return true;
    }
    return blendsOrderIndependently(draw) == (translucentDraws_ != TranslucentDraws::Remaining);
}

// the plain alpha blend (SRC_ALPHA / ONE_MINUS_SRC_ALPHA, add): additive glows and subtractive meshes do not depend on
// the order among themselves, and the Hud Lens draws after its own frame copy. A mesh laid over another (surface flag
// 0x20000: Draco's sand over its grass) keeps the game's order, drawn last over the composite as the game intends
bool SceneRecorder::blendsOrderIndependently(const Draw &draw)
{
    const argon::MeshState &st = draw.st;
    return st.blend && st.src == argon::gfx::BlendFactor::SrcAlpha && st.dst == argon::gfx::BlendFactor::OneMinusSrcAlpha && st.op == argon::gfx::BlendOp::Add && !draw.hudLens &&
           !st.laidOver;
}

// Argon Options' Order Independent, with the game's materials (without them nothing is translucent) and solid fill
bool SceneRecorder::usesOrderIndependence() const
{
    return sources_.translucency.active() && sources_.toggles.gameMaterials && !sources_.toggles.wireframe;
}

// the translucent pass after the water (0x938a0's bucket 4 and the props' second pass); with order independence the
// alpha-blended draws' solid pixels go into the scene and their other pixels into the OIT targets, laid over the scene
// before the other translucent draws
void SceneRecorder::drawTranslucentPass(float seconds)
{
    if (!usesOrderIndependence())
    {
        drawTranslucentDraws(TranslucentDraws::All, seconds);
        return;
    }
    drawTranslucentDraws(TranslucentDraws::SolidParts, seconds);
    sources_.translucency.begin();
    drawTranslucentDraws(TranslucentDraws::OrderIndependent, seconds);
    sources_.translucency.composite(commands_);
    boundPipe_ = {};
    drawTranslucentDraws(TranslucentDraws::OitDepth, seconds);
    drawTranslucentDraws(TranslucentDraws::Remaining, seconds);
    translucentDraws_ = TranslucentDraws::All;
}

void SceneRecorder::drawTranslucentDraws(TranslucentDraws which, float seconds)
{
    translucentDraws_ = which;
    drawModel(sources_.track, 1, {});
    drawProps(1, seconds);
}

void SceneRecorder::bindTextures(argon::gfx::TextureSet set)
{
    if (set == boundTextures_)
    {
        return;
    }
    commands_.bindTextures(set);
    boundTextures_ = set;
}

void SceneRecorder::drawMesh(const MeshCall &mesh, int pass)
{
    if (!drawsInPass(mesh.draw, pass))
    {
        return;
    }
    if (isMeshHidden(mesh))
    {
        return;
    }
    if (mesh.draw.hudLens && deferLenses_)
    {
        lenses_.push_back({&mesh.model, mesh.index, mesh.placement});
        return;
    }
    bindMeshState(mesh);
    bindTextures(mesh.draw.texSet);
    if (pass == 0)
    {
        bindCache_.empty(0);   // 0xbbc99: the opaque render list empties stage 0 before each of its meshes
    }
    const float occlusion = occlusionIntensity(mesh);
    if (occlusion <= 0.f)
    {
        return;
    }
    bindCache_.bindMesh(&mesh.model, mesh.draw.textures);
    const glm::mat4 world = mesh.draw.camera.drawn(listedWorld(mesh), viewPoint_);
    const glm::mat4 mvp = argon::liftedDepth(viewProj_ * world, argon::depthLiftOf(meshState(mesh.draw)));
    pushMatrices(mvp, shaderModel(mesh, world, occlusion));
    const uint32_t record = sources_.meshDraws.add(meshDrawOf(mesh));
    // firstInstance = the draw's record (gl_InstanceIndex in the shaders, mesh_draws.glsl)
    commands_.drawIndexed({uint32_t(mesh.draw.indexCount), uint32_t(mesh.draw.firstIndex), int32_t(mesh.draw.vertexOffset), record});
}

// its material, the craft's own lights and the bound stages' rates
MeshDraw SceneRecorder::meshDrawOf(const MeshCall &mesh) const
{
    MeshDraw record;
    record.material = int32_t(mesh.draw.matIndex);
    record.stageRates = boundStageRates();
    if (mesh.placement.lights)
    {
        record.ambient = glm::vec4(mesh.placement.lights->ambient, 1.f);
        record.light0 = glm::vec4(mesh.placement.lights->light0, 1.f);
    }
    return record;
}

// the rate each bound stage's frame was picked with: an earlier draw's when its bind still holds the slot (0xcbbd2)
glm::vec4 SceneRecorder::boundStageRates() const
{
    glm::vec4 rates(-1.f);
    for (int stage = 0; stage < argon::kTextureStages; ++stage)
    {
        const std::optional<argon::StageBind> bound = bindCache_.bound(stage);
        if (bound)
        {
            rates[stage] = bound->rate;
        }
    }
    return rates;
}

// ------------------------------------------------------------------ flare occlusion (0xb6e60)
// the test for the next frames, the intensity from the last result (0 before the first one: flare_occlusion.hpp). The
// intensity lives in the bone record (+0x138): a tested mesh writes its bone's, a mesh with surface flag 0x80000
// (lens-flare pieces under a FLARE_ bone) reads its parent bone's (1.0 from the load until the parent is tested)
float SceneRecorder::occlusionIntensity(const MeshCall &mesh)
{
    if (!sources_.toggles.gameMaterials)
    {
        return 1.f;
    }
    if (mesh.draw.occlTest)
    {
        return sources_.flares.active() ? testedOcclusion(mesh) : 1.f;
    }
    return mesh.draw.occlCopy ? parentOcclusion(mesh) : 1.f;
}

float SceneRecorder::parentOcclusion(const MeshCall &mesh) const
{
    const Track &model = mesh.model;
    const int bone = mesh.draw.bone;
    const int parent = bone < int(model.boneParent.size()) ? model.boneParent[size_t(bone)] : -1;
    if (parent < 0)
    {
        return 1.f;
    }
    return sources_.flares.knownIntensity(FlareQueries::key(&model, parent, -1, mesh.placement.occlusionInstance)).value_or(1.f);
}

float SceneRecorder::testedOcclusion(const MeshCall &mesh)
{
    const uint64_t key = FlareQueries::key(&mesh.model, mesh.draw.bone, int(mesh.index), mesh.placement.occlusionInstance);
    // once a frame per mesh: the OIT meshes' solid parts queue it (in the OIT pass the scene pass's point pipeline
    // cannot draw, and their depth stage would test it twice)
    const bool queues = translucentDraws_ != TranslucentDraws::OrderIndependent && translucentDraws_ != TranslucentDraws::OitDepth;
    if (queues && sources_.flares.hasRoom(sources_.frame))
    {
        queueOcclusionQuery(mesh, key);
    }
    return sources_.flares.testedIntensity(key);
}

// 0xa64f0: the first N points of a unit sphere, scaled by the surface radius, drawn depth tested inside a query
void SceneRecorder::queueOcclusionQuery(const MeshCall &mesh, uint64_t key)
{
    const argon::gfx::OcclusionQuery occlusion = sources_.flares.queue(sources_.frame, key, mesh.draw.occlPoints);
    glm::mat4 scaled = listedWorld(mesh);
    for (int axis = 0; axis < 3; ++axis)
    {
        scaled[axis] *= mesh.draw.occlRadius;
    }
    boundPipe_ = sources_.flares.pipeline();
    commands_.bindPipeline(boundPipe_);
    setDepthCull({.test = true, .write = false, .cullBack = false});
    bindVertexBuffer(sources_.flares.points());
    pushMatrices(viewProj_ * scaled, scaled);
    commands_.beginQuery(occlusion);
    commands_.draw({uint32_t(mesh.draw.occlPoints), 0});
    commands_.endQuery(occlusion);
    bindVertexBuffer(mesh.model.vb);
    bindMeshState(mesh);
}

// ------------------------------------------------------------------ the mesh's matrices
glm::mat4 SceneRecorder::baseWorld(const MeshCall &mesh) const
{
    const ModelPlacement &pl = mesh.placement;
    const glm::mat4 &placed = pl.onlyBone < 0 ? pl.instance : pl.boneMatrix;
    const size_t posed = size_t(mesh.draw.bone) * 16;
    if (pl.bonePose && mesh.draw.bone >= 0 && posed + 16 <= pl.bonePose->size())
    {
        return placed * glm::make_mat4(&(*pl.bonePose)[posed]);
    }
    return placed * mesh.draw.model;
}

// a flap bone turned by its ship class (craft_flaps.hpp); a jet bone scaled (1, 1, size) in its own space (0x50960)
glm::mat4 SceneRecorder::meshWorld(const MeshCall &mesh) const
{
    const ModelPlacement &pl = mesh.placement;
    const Track &model = mesh.model;
    if (isTurnedBone(mesh))
    {
        const size_t turned = size_t(pl.turnedBone);
        return pl.instance * model.boneRest[turned] * pl.boneTurn * model.boneRestInverse[turned] * mesh.draw.model;
    }
    const int jet = jetBoneOf(mesh);
    if (jet >= 0)
    {
        const glm::mat4 scale = glm::scale(glm::mat4(1), glm::vec3(1.f, 1.f, pl.jetSize));
        return pl.instance * model.boneRest[size_t(jet)] * scale * model.boneRestInverse[size_t(jet)] * mesh.draw.model;
    }
    const int side = flapSideOf(mesh);
    if (side >= 0)
    {
        return pl.instance * model.flaps.offset(side, pl.flaps->turn(*model.flaps.rule(), side)) * mesh.draw.model;
    }
    return baseWorld(mesh);
}

// the render list's camera step (0xb7e72); a single bone's draw (0xba100: the shots, render list argument 6 = 0)
// skips it
glm::mat4 SceneRecorder::listedWorld(const MeshCall &mesh) const
{
    const glm::mat4 world = meshWorld(mesh);
    if (mesh.placement.onlyBone >= 0 || !mesh.placement.cameraListed || !mesh.draw.camera.any())
    {
        return world;
    }
    return mesh.draw.camera.listed(world, viewPoint_);
}

bool SceneRecorder::isTurnedBone(const MeshCall &mesh) const
{
    const int bone = mesh.placement.turnedBone;
    return bone >= 0 && mesh.draw.bone >= 0 && mesh.model.underBone(mesh.draw.bone, bone);
}

int SceneRecorder::jetBoneOf(const MeshCall &mesh) const
{
    if (!hasJetData(mesh.model, mesh.draw.bone, mesh.placement.jetSize))
    {
        return -1;
    }
    return mesh.model.jetOf[size_t(mesh.draw.bone)];
}

int SceneRecorder::flapSideOf(const MeshCall &mesh) const
{
    if (!mesh.placement.flaps || !mesh.model.flaps.rule())
    {
        return -1;
    }
    return mesh.model.flaps.sideOf(mesh.draw.bone);
}

// 0x50960: a jet tinted (255, 128 s, 128 s, 400 s), a flare twice that; the tint (instance +0x104) multiplies the
// surface colour (setup 2, flag 4, 0xba1d0). The jets are unlit (flag 0x20): their tint has no effect
std::optional<glm::vec3> SceneRecorder::jetTint(const MeshCall &mesh) const
{
    const Track &model = mesh.model;
    if (!mesh.draw.tintLit || !hasJetData(model, mesh.draw.bone, mesh.placement.jetSize))
    {
        return std::nullopt;
    }
    const float jetSize = mesh.placement.jetSize;
    const float red = std::min(255.f, jetSize * 128.f), blue = std::min(255.f, jetSize * 400.f);
    if (model.jetOf[size_t(mesh.draw.bone)] >= 0)
    {
        return glm::vec3(red, red, blue) / 256.f;
    }
    if (!model.flareOf[size_t(mesh.draw.bone)])
    {
        return std::nullopt;
    }
    const float flareRed = std::min(255.f, 2.f * red) / 256.f;
    return glm::vec3(flareRed, flareRed, std::min(255.f, 2.f * blue) / 256.f);
}

// a placement's bone tint (the shields' colour fade) or a jet's, on lit meshes only
std::optional<glm::vec3> SceneRecorder::boneTint(const MeshCall &mesh) const
{
    if (mesh.placement.tint && mesh.draw.tintLit)
    {
        return mesh.placement.tint;
    }
    return jetTint(mesh);
}

// the tint goes to the shader in the model matrix's unused bottom row (w = 2 marks it); the occlusion intensity scales
// it (0xba1d0), a lit flare without a jet tint starting from 255; a Color Scale mesh takes its tinted colour lit or not
glm::mat4 SceneRecorder::shaderModel(const MeshCall &mesh, glm::mat4 world, float occlusion) const
{
    std::optional<glm::vec3> tint = mesh.draw.colorScale ? colorScaleTint(mesh.placement.tint, occlusion) : boneTint(mesh);
    const bool occluded = mesh.draw.occlTest || mesh.draw.occlCopy;
    if (occluded && mesh.draw.tintLit && !mesh.draw.colorScale)
    {
        tint = tint.value_or(glm::vec3(255.f / 256.f)) * occlusion;
    }
    if (!tint)
    {
        return world;
    }
    world[0][3] = tint->x;
    world[1][3] = tint->y;
    world[2][3] = tint->z;
    world[3][3] = 2.f;
    return world;
}

// ------------------------------------------------------------------ the race / editor scene
void SceneRecorder::drawWorld(const glm::mat4 &view, float fovDeg)
{
    const glm::mat4 viewProj = viewProj_;
    const double seconds = host_.secondsSinceStart();
    useViewPoint(view);
    drawSky();
    deferLenses_ = true;
    hiddenTrackBone_ = race_.hiddenNetGlowBone();
    findVisibleTrackMeshes();
    drawModel(sources_.track, 0, {});
    drawProps(0, float(seconds));
    host_.loadSprayTexture();
    const EffectSources sources{sources_.effectBuffers,
                                sources_.shadows,
                                sources_.waterPipe,
                                float(sources_.presenter.windowSize().height),
                                sources_.toggles,
                                sources_.spray,
                                sources_.weaponSprites,
                                sources_.particles,
                                sources_.track,
                                sources_.camera,
                                sources_.clock,
                                sources_.frame,
                                host_.viewSlot()};
    EffectPasses effects(sources, *this);
    effects.drawWater(seconds);
    effects.drawShadowReceivers();   // 0x938a0 (0x9c380): after the water, whose depth hides the shadows under it
    drawTranslucentPass(float(seconds));
    effects.drawEffects(view, fovDeg, seconds);
    deferLenses_ = false;
    drawHudLenses();
    if (sources_.driving && !race_.frontEndOpen())
    {
        host_.drawCountdown(*this);
    }
    useViewProj(viewProj);
    host_.drawEditorMarkings(*this);
}

// the frame so far copied (FrameCopies::copySceneForLenses), then the Hud Lens meshes over it
void SceneRecorder::drawHudLenses()
{
    if (lenses_.empty())
    {
        return;
    }
    sources_.frameCopies.copySceneForLenses();
    bindCache_.emptyAll();
    for (const DeferredLens &lens : lenses_)
    {
        bindModelBuffers(*lens.model);
        boundTextures_ = {};
        drawMesh(MeshCall{*lens.model, lens.model->draws[lens.index], lens.index, lens.placement}, 1);
    }
    lenses_.clear();
}

// in a race the track draw keeps the meshes of the sector nearest the eye and of its neighbours (visdata.vis, 0xafcf0)
void SceneRecorder::findVisibleTrackMeshes()
{
    visibleTrackMeshes_ = nullptr;
    const argon::TrackVisibility &visibility = sources_.track.data.visibility;
    if (!sources_.driving || visibility.empty())
    {
        return;
    }
    TrackSectorMeshes &seen = sources_.sectorMeshes[size_t(sources_.view)];
    const size_t sector = visibility.nearestSector(sources_.camera.eye);
    const size_t drawCount = sources_.track.draws.size();
    if (seen.trackSerial != sources_.trackSerial || seen.sector != sector || seen.drawCount != drawCount)
    {
        seen.trackSerial = sources_.trackSerial;
        seen.sector = sector;
        seen.drawCount = drawCount;
        seen.visible.assign(trackMeshCount(), false);
        visibility.markVisibleMeshes(sector, seen.visible);
    }
    visibleTrackMeshes_ = &seen.visible;
}

size_t SceneRecorder::trackMeshCount() const
{
    size_t meshCount = 0;
    for (const Draw &draw : sources_.track.draws)
    {
        meshCount = std::max(meshCount, size_t(draw.mesh) + 1);
    }
    return meshCount;
}

void SceneRecorder::drawSky()
{
    if (!sources_.toggles.groups[G_SKY] || sources_.raceEggs.on(Egg::AtlantisMode))   // 0x936c9: Atlantis Mode draws no sky
    {
        return;
    }
    const glm::mat4 viewProj = viewProj_;
    useViewProj(viewProj * glm::translate(glm::mat4(1), sources_.camera.eye));
    drawModelBothPasses(sources_.sky, ModelPlacement::forSky());
    useViewProj(viewProj);
}

void SceneRecorder::drawProps(int pass, float seconds)
{
    nextPropPose_ = 0;
    drawStoryActors(pass);
    drawGridShips(pass);
    drawGhosts(pass);
    drawPickups(pass, seconds);
    drawShots(pass, seconds);
    drawShields(pass);
    drawExplosions(pass);
}

// a story scene's actors at their sequence's world matrices (0x99360: SetTransform 0x10c2d0, then the model 0xb83e0),
// posed before the first pass
void SceneRecorder::drawStoryActors(int pass)
{
    if (!sources_.story->playing())
    {
        return;
    }
    if (pass == 0)
    {
        sources_.story->poseActors();
    }
    for (const StoryScene::ActorDraw &actor : sources_.story->actorDraws())
    {
        ModelPlacement placement = ModelPlacement::forProp(actor.world);
        placement.cameraListed = false;
        drawModel(*actor.model, pass, placement);
    }
}

// ships on the grid (the start matrix places them; slot 0 follows the drive)
void SceneRecorder::drawGridShips(int pass)
{
    if (!sources_.toggles.ships)
    {
        return;
    }
    const bool cockpit = sources_.driving && sources_.chaseCam && sources_.driveView.view() == DriveCamera::kCockpit;
    for (size_t i = 0; i < sources_.track.data.starts.size(); ++i)
    {
        const Found<Track> ship = sources_.props.loadedCraft(sources_.startGrid.pilotOf(int(i)));
        if (!ship || !race_.slotShown(int(i)))
        {
            continue;
        }
        if (i == 0 && cockpit)
        {
            continue;   // cockpit view: own ship hidden
        }
        ModelPlacement pl = ModelPlacement::forProp(sources_.poses.slotMatrix(int(i)));
        const bool tracked = i < size_t(Weapons::kShips);
        pl.jetSize = tracked ? std::max(sources_.exhausts.jetSize(int(i)), 0.1f) : 0.1f;
        pl.flaps = tracked ? &sources_.exhausts.flaps(int(i)) : nullptr;
        pl.occlusionInstance = int(i) + 1;
        pl.lights = craftLightsOf(int(i));
        drawModel(*ship, pass, pl);
    }
}

// a racing craft's lights from the track colour under it (0x59bc0); a grid ship without a craft, or one that has not
// hovered yet (no colour: alpha 0), keeps the track's
std::optional<argon::SceneLights> SceneRecorder::craftLightsOf(int slot) const
{
    const Found<const argon::GameCraft> craft = sources_.race.craftInSlot(slot);
    if (!craft || (craft->groundColour() >> 24) == 0)
    {
        return std::nullopt;
    }
    const auto &props = sources_.track.data.props;
    argon::SceneLights track;
    track.ambient = glm::vec3(props.ambient[0], props.ambient[1], props.ambient[2]);
    track.light0 = glm::vec3(props.sunColour[0], props.sunColour[1], props.sunColour[2]);
    return argon::craftLights(craft->groundColour(), track);
}

// Time Attack's ghosts (logic\time_attack.cpp): drawn as any craft, opaque (checked in the game: same bone tints, flags
// and draw path as the player's craft)
void SceneRecorder::drawGhosts(int pass)
{
    if (!sources_.toggles.ships)
    {
        return;
    }
    const TimeAttackLogic &timeAttack = sources_.modes.timeAttack();
    for (int g = 0; g < 2; ++g)
    {
        const Found<Track> model = race_.shownGhostModel(g);
        if (!model)
        {
            continue;
        }
        ModelPlacement pl = ModelPlacement::forProp(timeAttack.ghostMatrix(g, sources_.poses.simAlpha()));
        pl.jetSize = std::max(timeAttack.ghostJet(g), 0.1f);
        pl.occlusionInstance = Weapons::kShips + 1 + g;
        drawModel(*model, pass, pl);
    }
}

// as the game: pup.txt Y + the type's lift + bob 0.5 + 0.5 sin(2t + index), no turn (pickup draw FUN_0008fbc0), the
// models' own tracks on the clock (0xb83e0); taken this lap: the _ns model (pickups_draw); taken and not back yet
// (0.4 s; power chips: gone): not drawn. A story scene over the track shows its actors only, no pickups
void SceneRecorder::drawPickups(int pass, float seconds)
{
    if (!sources_.toggles.pickupModels || !race_.modeHasPickups() || sources_.story->playing())
    {
        return;
    }
    if (pass == 0)
    {
        host_.animatePickupModels(seconds);
    }
    const argon::Pickups &pickups = sources_.pickups;
    for (size_t i = 0; i < pickups.count(); ++i)
    {
        if (!pickups.onTrack(i))
        {
            continue;
        }
        const auto &models = pickups.takenThisLap(i) ? sources_.pickupTakenFor : sources_.pickupFor;
        auto it = models.find(pickups.typeName(i));
        if (it == models.end() || !it->second)
        {
            continue;
        }
        glm::vec3 pos = pickups.position(i);
        pos.y += 0.5f + 0.5f * std::sin(seconds * 2.f + float(i));
        drawModel(*it->second, pass, ModelPlacement::forProp(glm::translate(glm::mat4(1), pos)));
    }
}

// weapon models (0x64e20): a shot draws the bone "<name><k>" of its ship's model at the shot's own matrix, a beam
// with its grey tint. The launch detaches that bone (+0x102 flag 2, 0x65124) and the ship's draw (0x59f6f -> 0xb83e0)
// sets the model's clock +0x5e4 to the race clock, so the bone's tracked children (Abi's XSTAR, Leah's AVALANCHE,
// Kukai's sphere4, Kumiko's xx1DATABOMB0 / XXX) play their tracks in flight, posed under the detached bone
void SceneRecorder::drawShots(int pass, float seconds)
{
    if (!sources_.weaponsOn)
    {
        return;
    }
    for (const auto &shot : sources_.weapons.shots())
    {
        const Found<Track> ship = sources_.props.loadedCraft(sources_.startGrid.pilotOf(shot.shooter));
        if (!ship)
        {
            continue;
        }
        if (shot.bone < 0)
        {
            continue;
        }
        ModelPlacement pl = shotPlacement(*ship, shot.bone, shotMatrix(shot), seconds, pass);
        if (isBeam(shot))
        {
            pl.tint = beamTint(shot);
        }
        drawModel(*ship, pass, pl);
    }
}

// 0x68620: the beam's bone tint (k, k, k, 255) through 0xb9930, k its grey (the fade over its life); lit meshes only,
// an additive one fading to black
glm::vec3 SceneRecorder::beamTint(const argon::weapons::Shot &shot) const
{
    return boneTintOf(argon::weapons::greyFlashColour(argon::weapons::beamFade(sources_.clock - shot.born, shot.weaponType->life)));
}

ModelPlacement SceneRecorder::shotPlacement(const Track &ship, int bone, const glm::mat4 &shot, float seconds, int pass)
{
    if (!ship.anim)
    {
        return ModelPlacement::forBone(bone, shot * ship.boneRestInverse[size_t(bone)]);
    }
    const std::vector<float> &pose = propPose(*ship.anim, seconds, pass);
    ModelPlacement pl = ModelPlacement::forBone(bone, shot * glm::inverse(glm::make_mat4(&pose[size_t(bone) * 16])));
    pl.bonePose = &pose;
    return pl;
}

// an active TSHIELD draws its bone on the ship
void SceneRecorder::drawShields(int pass)
{
    if (!sources_.weaponsOn)
    {
        return;
    }
    for (int racer = 0; racer < sources_.race.racerCount() && racer < Weapons::kShips; ++racer)
    {
        const int slot = sources_.race.slotOf(racer);
        if (sources_.weapons.shieldActive(slot))
        {
            drawShield(slot, pass);
        }
    }
}

// 0x69970: the shield's bones run their animation tracks on its age (+0x188, 0xb9e50: 1SHIELD_TARA0 on every craft)
// and take its colour fade as their tint (+0x104, 0xb9930)
void SceneRecorder::drawShield(int slot, int pass)
{
    const Found<Track> ship = sources_.props.loadedCraft(sources_.startGrid.pilotOf(slot));
    const Weapons::ActiveShield &shield = sources_.weapons.craft(slot).shield();
    const int bone = shield.bone;
    if (!ship || bone < 0)
    {
        return;
    }
    const float age = sources_.clock - shield.since;
    ModelPlacement pl = ModelPlacement::forBone(bone, sources_.poses.slotMatrix(slot));
    if (ship->anim)
    {
        pl.bonePose = &propPose(*ship->anim, age, pass);
    }
    pl.tint = boneTintOf(argon::weapons::shieldTint(shield.weaponType->fade, age));
    drawModel(*ship, pass, pl);
}

// 0x7de80: each explosion between its start and its end, its model at its matrix (turned, scaled), every bone's track
// at the progress x the type's clock factor (own clocks, 0x7dd40 / 0xb9f30), every bone's tint a grey falling to
// black (0xb99b0; lit meshes only); drawn through the render list (0xb83e0 with argument 1: the camera step applies)
void SceneRecorder::drawExplosions(int pass)
{
    for (const Weapons::Explosion &explosion : sources_.weapons.explosions().inFlight())
    {
        const Found<Track> model = sources_.props.loadedExplosion(explosion.type);
        if (!model || !argon::weapons::explosionShows(explosion.due, explosion.duration, sources_.clock))
        {
            continue;
        }
        const float progress = argon::weapons::explosionProgress(explosion.due, explosion.duration, sources_.clock);
        ModelPlacement pl = ModelPlacement::forProp(argon::weapons::explosionMatrix(explosion.at, explosion.turnDegrees, explosion.scale));
        if (model->anim)
        {
            pl.bonePose = &propPose(*model->anim, progress * sources_.weapons.book().explosionType(explosion.type).look.clockFactor, pass);
        }
        pl.tint = boneTintOf(argon::weapons::explosionGrey(progress));
        drawModel(*model, pass, pl);
    }
}

// pass 0 poses the model and keeps the pose; pass 1 draws the same props in the same order, so it takes pass 0's
// poses back in turn instead of posing every animated shot, shield and explosion a second time
const std::vector<float> &SceneRecorder::propPose(const argon::Cmm &anim, float seconds, int pass)
{
    const size_t slot = nextPropPose_++;
    const bool unseen = slot >= propPoses_.size();
    if (unseen)
    {
        propPoses_.emplace_back();
    }
    std::vector<float> &pose = propPoses_[slot];
    if (pass == 0 || unseen)
    {
        anim.pose(seconds, pose);
    }
    return pose;
}

// ------------------------------------------------------------------ the front end's models (from the ImGui callback)
ModelPlacement SceneRecorder::frontEndPlacement(const FeScene &scene, int model)
{
    ModelPlacement pl = ModelPlacement::forProp(scene.placement * scene.offsets[model]);
    pl.cameraListed = false;
    if (model == 0 && scene.turnedBone)
    {
        pl.turnedBone = scene.models[0]->findBone(scene.turnedBone);
        pl.boneTurn = scene.boneTurn;
    }
    return pl;
}

// the game draws a 640 x 480 screen; keep its vertical fov and widen the view for wider windows
void SceneRecorder::drawFrontEndModels(const FeScene &scene, float aspect)
{
    if (scene.empty() || !scene.models[0]->vb)
    {
        return;
    }
    const argon::gfx::Rect area =
        scene.boxed ? argon::gfx::Rect{int32_t(scene.box.x), int32_t(scene.box.y), uint32_t(scene.box.w), uint32_t(scene.box.h)}
                    : argon::gfx::Rect{0, 0, sources_.presenter.windowSize().width, sources_.presenter.windowSize().height};
    commands_.clearDepth(area, argon::kDepthClear);
    setViewport(area);
    commands_.setFrontFace(host_.modelFrontFace());   // the menus' camera is never mirrored
    commands_.bindScene(sources_.sceneBuffers.set(host_.viewSlot()));
    useViewProj(argon::reversedPerspective(scene.fovY(), aspect, scene.zNear, 5000.f) * scene.view());
    useViewPoint(scene.view());
    for (int pass = 0; pass < 2; ++pass)
    {
        for (int i = 0; i < 2; ++i)
        {
            if (Track *model = scene.models[i]; model && model->vb)
            {
                drawModel(*model, pass, frontEndPlacement(scene, i));
            }
        }
    }
}
