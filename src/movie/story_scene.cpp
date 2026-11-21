// Argon: a story scene playing (movie\story_scene.hpp, docs\sequences\scenes.md "Story scenes")
#include "movie/story_scene.hpp"

#include <cstdio>
#include <glm/gtc/type_ptr.hpp>

#include "audio/audio.hpp"
#include "files/read_bytes.hpp"
#include "render/frame_clocks.hpp"
#include "render/model_builder.hpp"
#include "saves/viewer_settings.hpp"
#include "ui/menu_kit.hpp"
#include "movie/nms_sequence.hpp"

#include <GLFW/glfw3.h>
#include "files/game_path.hpp"
#include "files/string_table.hpp"
#include "movie/head_turn.hpp"
#include "movie/story_scenes.hpp"
#include "ui/menu_common.hpp"

namespace
{
constexpr float kEndBefore = 0.1f;   // 0x9aa00: ends once the first track's length less 0.1 s has played (0x1a3508)
// 0x98c30 with global_cutscene_bars_enabled (set by 0x9a810): the letterbox, black (-100, -100)..(840, 56) and
// (-100, 424)..(840, 580)
constexpr float kBarTop = 56.f, kBarBottom = 424.f;
// the subtitle: the panel 0xc1b40 at (margin, 300) to the other margin, 100 high, fill 0xa0ffa030, plasma 0.4; the
// speaker's portrait 84 x 84 at (margin + 10, 305); the text box +0x125c at (margin + 96, 310), 640 - 2 margins - 128
// wide (0x9a020), white
constexpr float kPanelY = 300.f, kPanelH = 100.f, kPortraitX = 10.f, kPortraitY = 305.f, kPortraitSize = 84.f;
constexpr float kTextX = 96.f, kTextY = 310.f, kTextInset = 128.f;
constexpr uint32_t kPanelLine = 0xffffff00u, kPanelFill = 0xa0ffa030u, kWhite = 0xffffffffu;
constexpr float kPanelPlasma = 0.4f;
constexpr float kActorAlphaRef = 0.5f;   // as the front end's characters (ui\fe_models.cpp)
} // namespace

StoryScene::StoryScene(StoryHost &host, const Sources &sources) : host_(host), sources_(sources)
{
}

StoryScene::~StoryScene() = default;

// 0x9a810 / 0x9a020: the sequence, then per object its model (0x96be0) or speech line (0x96a60); cutscenes.str
// (0x9a7e9)
bool StoryScene::start(const std::string &trackFolder, const std::string &file)
{
    stop();
    const std::string path = sources_.gameFolder + "/data/movies/" + trackFolder + "/" + file;
    if (!argon::gamePathExists(path) || !sequence_.load(readBytes(argon::findGamePath(path).string())))
    {
        std::printf("story scene %s: not played (missing or unreadable)\n", file.c_str());
        return false;
    }
    loadSubtitles();
    loadActors();
    loadSpeech();
    playing_ = true;
    endedAt_ = -1.f;
    startedAt_ = glfwGetTime() - sources_.startAt;
    voiceObject_ = shownObject_ = cameraObject_ = -1;
    std::printf("story scene %s: %.2f s, %zu actors, %zu speeches\n", file.c_str(), sequence_.length(), actors_.size(), speeches_.size());
    sources_.audio.playSceneAmbience();
    update();
    return true;
}

void StoryScene::stop()
{
    if (playing_)
    {
        sources_.audio.stopMovie();
        sources_.audio.stopSceneAmbience();
    }
    playing_ = false;
    actors_.clear();
    speeches_.clear();
}

void StoryScene::unloadModels()
{
    for (auto &kv : models_)
    {
        sources_.models.release(*kv.second);
    }
    models_.clear();
}

// 0x9af10: A or START while it plays: the voice stops, the scene ends
void StoryScene::skip()
{
    if (finished())
    {
        return;
    }
    sources_.audio.stopMovie();
    endedAt_ = seconds();
}

float StoryScene::seconds() const
{
    return finished() ? endedAt_ : float(glfwGetTime() - startedAt_);
}

// 0x9aa00 -> seq_update 0x98b70: the camera (a new one fades, 0x97486), the actors' animations, the speech; once ended
// nothing moves (seq_update is not called again)
void StoryScene::update()
{
    if (!playing_ || finished())
    {
        return;
    }
    const SequenceFrame frame = sequence_.frameAt(seconds());
    if (frame.camera)
    {
        host_.useSequenceCamera(*frame.camera, cameraObject_);
    }
    for (Actor &actor : actors_)
    {
        for (const SequenceActor &state : frame.actors)
        {
            actor.state = state.object == actor.object ? state : actor.state;
        }
    }
    playSpeech(frame.speech);
    for (Actor &actor : actors_)
    {
        bindAnimations(actor);
    }
    if (seconds() >= sequence_.length() - kEndBefore)
    {
        endedAt_ = seconds();
    }
}

// 0x96dd0: a new speech stops the last voice and starts its own; nothing (or a group) stops it and hides the subtitle
void StoryScene::playSpeech(const SequenceSpeech &speech)
{
    if (speech.object < 0)
    {
        sources_.audio.stopMovie();
        shownObject_ = -1;
        speechSeconds_ = 0.f;
        return;
    }
    if (speech.object != voiceObject_)
    {
        sources_.audio.stopMovie();
        const auto it = speeches_.find(speech.object);
        if (it != speeches_.end() && it->second.voice)
        {
            sources_.audio.playMovie(it->second.voice);
            std::printf("story speech %s at %.2f s: %s (%.2f s)\n", it->second.line.key, seconds(), it->second.line.wav, it->second.voice->seconds());
        }
        voiceObject_ = speech.object;
    }
    shownObject_ = speech.object;
    speechSeconds_ = speech.seconds;
}

// 0x99360 per model: the body animation bound by bone name when it changes (0xba060 -> 0xb55e0); the speech showing
// binds its face animation to its speaker's face bones (0xb5710) on their own clock, the speech's time (0xb9ec0)
void StoryScene::bindAnimations(Actor &actor)
{
    if (!actor.model || !actor.model->anim)
    {
        return;
    }
    const argon::Cmm &cmm = *actor.model->anim;
    if (actor.state.animation >= 0 && actor.state.animation != actor.boundAnimation)
    {
        if (const argon::CmsAnimation *animation = sequence_.animation(actor.state.animation))
        {
            actor.body = argon::tracksByBone(cmm, *animation);
        }
        actor.boundAnimation = actor.state.animation;
    }
    const auto it = speeches_.find(shownObject_);
    if (it == speeches_.end() || it->second.face.tracks.empty() || it->second.line.speaker != actor.pilot)
    {
        return;
    }
    if (actor.faceObject != shownObject_)
    {
        actor.face = argon::tracksByBone(cmm, it->second.face);
        actor.faceObject = shownObject_;
    }
    actor.faceSeconds = speechSeconds_;
}

// every bone plays once unless the segment loops (0xba000 sets bone flag 0x400 on all); a face bone takes the face
// animation (flag 0x1000), else the body's; before any animation the model's own tracks at the clock (0x99633)
std::vector<argon::BoneKeys> StoryScene::boneDrivers(const Actor &actor) const
{
    const argon::Cmm &cmm = *actor.model->anim;
    const bool once = !actor.state.loops;
    std::vector<argon::BoneKeys> drivers =
        actor.boundAnimation < 0 ? cmm.ownTracks(float(sources_.clocks.secondsSinceStart())) : std::vector<argon::BoneKeys>(cmm.bones.size());
    for (size_t b = 0; b < drivers.size(); ++b)
    {
        drivers[b].once = once;
        if (b < actor.body.size() && actor.body[b])
        {
            drivers[b] = argon::BoneKeys{actor.body[b], actor.state.seconds, once};
        }
        if (b < actor.face.size() && actor.face[b])
        {
            drivers[b] = argon::BoneKeys{actor.face[b], actor.faceSeconds, once};
        }
    }
    return drivers;
}

// in slot order, as 0x99360 draws them: a head turned to an actor posed earlier this frame sees where it is now, one
// posed later where it was last frame
void StoryScene::poseActors()
{
    for (Actor &actor : actors_)
    {
        if (actor.model && actor.model->anim)
        {
            sources_.models.animateWith(*actor.model, argon::PoseDrivers{boneDrivers(actor), headAims(actor)}, sources_.clocks.modelClock(seconds()));
            notePosedHead(actor);
        }
    }
}

// 0x99360 / 0x999b5: the head, neck and eyes turn towards the point in the model's own space (the bones pose there)
std::vector<argon::BoneAim> StoryScene::headAims(const Actor &actor) const
{
    if (actor.state.lookAt < 0)
    {
        return {};
    }
    const glm::vec3 target = glm::vec3(glm::inverse(actor.state.world) * glm::vec4(lookTarget(actor), 1.f));
    std::vector<argon::BoneAim> aims = argon::story::headTurnAims(*actor.model->anim, target, actor.state.lookShare);
    const float modelScale = glm::length(glm::vec3(actor.state.world[0]));
    for (argon::BoneAim &aim : aims)
    {
        aim.modelScale = modelScale;
    }
    return aims;
}

// 0x99908: an actor looked at gives its head bone's last posed position (+0xc0, 0x997a2), else the object's own
glm::vec3 StoryScene::lookTarget(const Actor &actor) const
{
    for (const Actor &other : actors_)
    {
        if (actor.state.looksAtActor && other.object == actor.state.lookAt && other.headPosed)
        {
            return other.headPoint;
        }
    }
    return actor.state.lookPoint;
}

void StoryScene::notePosedHead(Actor &actor) const
{
    const std::vector<float> &bones = actor.model->animWorld;
    if (actor.headBone < 0 || size_t(actor.headBone) * 16 + 16 > bones.size())
    {
        return;
    }
    actor.headPoint = glm::vec3(actor.state.world * glm::make_mat4(&bones[size_t(actor.headBone) * 16])[3]);
    actor.headPosed = true;
}

std::vector<StoryScene::ActorDraw> StoryScene::actorDraws() const
{
    std::vector<ActorDraw> draws;
    for (const Actor &actor : actors_)
    {
        if (actor.model && actor.model->vb)
        {
            draws.push_back({actor.model, actor.state.world});
        }
    }
    return draws;
}

// 0x96be0: characters\<folder> (model.cmm and its textures), kept for later scenes
Track *StoryScene::loadModel(const std::string &folder)
{
    auto &slot = models_[folder];
    if (!slot)
    {
        slot = std::make_unique<Track>();
        slot->name = folder;
        slot->translucentAlphaRef = kActorAlphaRef;
        slot->posable = true;
        sources_.models.load(
            *slot, sources_.gameFolder + "/data/characters/" + folder, ModelUse{.pool = sources_.propPool, .prop = true, .reskinned = true});
    }
    return slot->vb ? slot.get() : nullptr;
}

// the type 2 (actor) and 10 (ship) objects
void StoryScene::loadActors()
{
    const std::vector<SequenceObject> objects = sequence_.objects();
    for (int i = 0; i < int(objects.size()); ++i)
    {
        const SequenceObject &object = objects[size_t(i)];
        if (object.type != NmsSequence::kModel && object.type != NmsSequence::kShipModel)
        {
            continue;
        }
        Actor actor;
        actor.object = i;
        actor.pilot = argon::story::pilotOfFolder(object.fileName);
        actor.model = loadModel(object.fileName);
        actor.headBone = actor.model && actor.model->anim ? argon::story::lookedAtBone(*actor.model->anim) : -1;
        actors_.push_back(std::move(actor));
    }
}

// 0x96a60 per type 9 object: its line (by the key at +0x160), the WAV, the face animation, the subtitle
void StoryScene::loadSpeech()
{
    const std::vector<SequenceObject> objects = sequence_.objects();
    for (int i = 0; i < int(objects.size()); ++i)
    {
        if (objects[size_t(i)].type != NmsSequence::kSpeech)
        {
            continue;
        }
        const std::optional<argon::story::SpeechLine> line = argon::story::speechLineFor(objects[size_t(i)].fileName);
        if (!line)
        {
            continue;
        }
        Speech speech{*line, std::make_shared<argon::Pcm>(), {}, subtitles_[line->stringId]};
        try
        {
            *speech.voice = argon::loadSound(argon::findGamePath(sources_.gameFolder + "/data/movies/speech/" + line->wav));
        }
        catch (const std::runtime_error &e)
        {
            std::printf("story speech %s: %s\n", line->wav, e.what());
            speech.voice.reset();
        }
        if (*line->faceAnimation)
        {
            const std::vector<uint8_t> face = readBytes(argon::findGamePath(sources_.gameFolder + "/data/characters/" + line->faceAnimation).string());
            argon::readCms(face, 0, speech.face);
        }
        speeches_[i] = std::move(speech);
    }
}

void StoryScene::loadSubtitles()
{
    if (subtitles_.empty())
    {
        subtitles_ = argon::parseStringTable(readBytes(sources_.gameFolder + "/" + sources_.settings.localeFolder() + "/cutscenes.str"));
    }
}

void StoryScene::drawOverlay(const FeCanvas &canvas) const
{
    if (!playing_)
    {
        return;
    }
    drawBars(canvas);
    drawSubtitle(canvas);
}

void StoryScene::drawBars(const FeCanvas &canvas) const
{
    ImDrawList *drawList = canvas.drawList();
    const float pixelsPerUnit = canvas.scale();
    const ImVec2 size = ImGui::GetIO().DisplaySize;
    drawList->AddRectFilled(ImVec2(0, 0), ImVec2(size.x, kBarTop * pixelsPerUnit), IM_COL32(0, 0, 0, 255));
    drawList->AddRectFilled(ImVec2(0, kBarBottom * pixelsPerUnit), size, IM_COL32(0, 0, 0, 255));
}

void StoryScene::drawSubtitle(const FeCanvas &canvas) const
{
    const float originX = canvas.originX();
    const auto it = speeches_.find(shownObject_);
    if (it == speeches_.end())
    {
        return;
    }
    using namespace menu;
    const float left = atLeft(originX, kSafeMargin), width = spanWidth(originX, 640.f - 2.f * kSafeMargin);
    canvas.panel({left, kPanelY, width, kPanelH}, {false, false, kPanelLine, true, kPanelFill, kPanelFill, kPanelPlasma});
    const Speech &speech = it->second;
    sources_.menus.drawPilotPortrait(canvas, speech.line.speaker, Box{left + kPortraitX, kPortraitY, kPortraitSize, kPortraitSize});
    if (finished())
    {
        return;   // 0x9ac31: once ended the text boxes are emptied; the panel and portrait stay
    }
    float y = kTextY;
    for (const std::u16string &line : sources_.menus.wrapText(speech.subtitle, 0, 1.f, width - kTextInset))
    {
        canvas.text({left + kTextX, y}, line, {0, kWhite, 0, 1.f, 1.f});
        y += sources_.menus.fontLine(0);
    }
}
