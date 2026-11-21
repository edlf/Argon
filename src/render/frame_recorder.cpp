// Argon: one frame's commands (render\frame_recorder.hpp): the scene pass's clear (0x93777), each view's own area
// (0x93717) and the front end's page models with their lights.
#include "render/frame_recorder.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <string>

#include "effects/engine_trails.hpp"
#include "logic/race_eggs.hpp"
#include "movie/movie_player.hpp"
#include "movie/story_scene.hpp"
#include "render/frame_copies.hpp"
#include "render/model_builder.hpp"
#include "render/projection.hpp"
#include "render/renderer.hpp"
#include "render/scene_constants.hpp"
#include "render/scene_recorder.hpp"
#include "render/scene_sources.hpp"
#include "ui/fe_shell.hpp"

namespace
{
const argon::gfx::Colour kBlack{0, 0, 0, 1};
const argon::gfx::Colour kNoFogClear{0.12f, 0.12f, 0.14f, 1};

bool endsWith(const std::string &text, const std::string &end)
{
    return text.size() >= end.size() && text.compare(text.size() - end.size(), end.size(), end) == 0;
}

// 0x178a0: the box's camera looks at the posed skel_head bone (0xb5190 finds it by name: "<pilot>_skel_head")
void aimBoxAtHead(FeScene &box)
{
    const Track &model = *box.models[0];
    if (!model.anim)
    {
        return;
    }
    for (size_t b = 0; b < model.anim->bones.size(); ++b)
    {
        if (endsWith(model.anim->bones[b].name, "skel_head") && b * 16 + 16 <= model.animWorld.size())
        {
            box.box.target = glm::vec3(glm::make_mat4(&model.animWorld[b * 16])[3]);
            return;
        }
    }
}
}  // namespace

void FrameRecorder::record()
{
    Renderer &renderer = sources_.renderer;
    argon::gfx::CommandList &commands = renderer.presenter().commands();
    renderer.beginFrame(commands, sources_.frame);
    host_.recordShadows(commands);
    recordTrails();
    sources_.movie->recordUpload(commands, sources_.frame);
    const ViewMoment moment = host_.viewMoment();
    renderer.presenter().beginScenePass(scenePassClear(sources_.cameras.viewCount(moment)));
    SceneRecorder recorder(sources_.scene, sources_.raceView, host_.sceneSources(), commands);
    recordRaceViews(recorder, moment);
    if (!host_.frontEndScreenShown())
    {
        renderer.recordPostAntiAliasing(commands);
    }
    renderer.ui().render();
    sources_.menuModels = {};
    renderer.presenter().endScenePass();
    sources_.frameCopies.keepFinishedFrame();
}

void FrameRecorder::recordTrails()
{
    if (sources_.trailsShown)
    {
        sources_.renderer.writeTrails(sources_.frame, sources_.engineTrails);
        return;
    }
    sources_.renderer.clearTrails();
}

// each view in its part of the screen with its camera; the HUD and the markers keep the first view's camera after
void FrameRecorder::recordRaceViews(SceneRecorder &recorder, const ViewMoment &moment)
{
    const std::vector<RaceView> views = sources_.cameras.raceViews(moment, sources_.renderer.presenter().windowSize());
    for (size_t i = 0; i < views.size(); ++i)
    {
        sources_.view = int(i);
        recordView(recorder, views[i], views.size() > 1);
    }
    sources_.view = 0;
    sources_.hudCamera = ViewCameras::hudCameraOf(views.front().camera);
}

void FrameRecorder::recordView(SceneRecorder &recorder, const RaceView &view, bool split)
{
    const argon::gfx::Rect area = screenArea(sources_.renderer.presenter().windowSize(), view.area);
    recorder.setViewport(area);
    if (split)
    {   // 0x93717: the view's own area to the scene colour, its depth to far
        recorder.commands().clear(area, argon::gfx::SceneClear{sceneClearColour(), argon::kDepthClear});
    }
    sources_.hudCamera = ViewCameras::hudCameraOf(view.camera);
    writeSceneConstants(view.camera.view);
    recorder.commands().bindScene(sources_.renderer.sceneSet(sources_.scene.viewSlot()));
    recorder.useViewProj(view.camera.proj * view.camera.view);
    if (host_.frontEndScreenShown())
    {
        prepareFrontEndModels(recorder);
        return;
    }
    recorder.drawWorld(view.camera.view, view.camera.fovDeg);
}

void FrameRecorder::writeSceneConstants(const glm::mat4 &viewMatrix)
{
    SceneMoment moment{viewMatrix, sources_.hudCamera.eye, float(sources_.scene.secondsSinceStart()), std::nullopt};
    if (sources_.story->playing())
    {
        moment.light1 = sources_.story->light();
    }
    sources_.renderer.writeSceneConstants(sources_.scene.viewSlot(), raceSceneConstants(sources_.track.data.props, sources_.toggles, moment));
}

// the front end's lights (render\scene_constants.hpp); the scene constants keep camera 1's view, the page's models use
// the page's camera
void FrameRecorder::writeFrontEndSceneConstants(const FeScene &scene)
{
    const FrontEndLights lights{FeScene().view(), scene.lightDir, scene.lightsOff, float(sources_.fe.frameCount()) / 60.f};
    sources_.renderer.writeSceneConstants(sources_.scene.viewSlot(), frontEndSceneConstants(lights, sources_.toggles));
}

// the front end (ui\fe_shell.hpp): only the current page's models, drawn from its ImGui callback; the background is
// the echo of the last frame (drawn by the shell)
void FrameRecorder::prepareFrontEndModels(SceneRecorder &recorder)
{
    const FeScene scene = sources_.fe.scene();
    writeFrontEndSceneConstants(scene);
    for (int i = 0; i < 2; ++i)
    {
        if (scene.models[i] && scene.models[i]->anim && scene.clocks[i] >= 0.f)
        {
            sources_.models.animate(*scene.models[i], sources_.scene.modelClock(scene.clocks[i]));
        }
    }
    const float aspect = windowAspect(sources_.renderer.presenter().windowSize());
    std::vector<FeScene> boxes = sources_.fe.boxScenes();
    for (FeScene &box : boxes)
    {
        sources_.models.animate(*box.models[0], sources_.scene.modelClock(box.clocks[0]));
        aimBoxAtHead(box);
    }
    sources_.menuModels = {&recorder, scene, aspect, std::move(boxes)};
}

argon::gfx::Colour FrameRecorder::sceneClearColour() const
{
    if (host_.frontEndScreenShown() || sources_.raceEggs.on(Egg::AtlantisMode))
    {
        return kBlack;   // Atlantis: the game's clear (0x93777) is black, no sky over it
    }
    const auto &fog = sources_.track.data.props.fogColour;
    return sources_.toggles.fog ? argon::gfx::Colour{fog[0], fog[1], fog[2], 1} : kNoFogClear;
}

// split screen: black between the views (each view clears its own area, recordView)
argon::gfx::SceneClear FrameRecorder::scenePassClear(int viewCount) const
{
    return argon::gfx::SceneClear{viewCount > 1 ? kBlack : sceneClearColour(), argon::kDepthClear};
}
