// Argon: a story scene playing (the scene page 0x585740, vtable 0x19d858: enter 0x9a810, update 0x9aa00, input 0x9af10;
// its view 0x98c30 / 0x99360; docs\sequences\scenes.md "Story scenes"). The NMS sequence's cameras over the loaded track, its
// actors (data\characters\<pilot> posed by the sequence's CMS animations and the speech's face animations), the
// speech (data\movies\speech) with its subtitle (cutscenes.str) in a panel under the pilot's portrait, letterboxed;
// the actors' heads turn towards what their segments name (movie\head_turn.hpp), the scene's own light joins the
// track's (light()). It ends at the first track's end or on A / START, then holds its last frame (no subtitle text). Where it plays: ui\loader.cpp (before a
// tournament race) and App::quitRaceTo (after a tournament's last race).
#pragma once
#include "ui/fe_canvas.hpp"
#include "gfx/gfx_types.hpp"
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <glm/glm.hpp>

#include "model/cms_animation.hpp"
#include "movie/nms_sequence.hpp"
#include "movie/scene_light.hpp"
#include "movie/speech_lines.hpp"

class FrameClocks;
class MenuKit;
class ModelBuilder;
class Renderer;
class ViewerSettings;
struct ModelClock;
struct SequenceCamera;
struct Track;
namespace argon
{
class Audio;
}
struct ImDrawList;
namespace argon
{
struct Pcm;
}

// what the scene asks App for: a model's clock, the frame's clock, the view's camera, the menus' pieces
class StoryHost
{
  public:
    virtual void useSequenceCamera(const SequenceCamera &camera, int &shownCamera) = 0;   // the view; a new camera fades

  protected:
    ~StoryHost() = default;
};

class StoryScene
{
  public:
    struct Sources
    {
        argon::Audio &audio;
        const std::string &gameFolder;
        ModelBuilder &models;
        const argon::gfx::SetPool &propPool;   // the actors load into it
        const ViewerSettings &settings;     // the subtitles' language
        const float &startAt;               // --story-at: the scene's clock starts there
        const FrameClocks &clocks;          // the actors' poses, their own tracks' clock
        MenuKit &menus;                     // the subtitles' portrait, wrap and line height
    };
    StoryScene(StoryHost &host, const Sources &sources);
    ~StoryScene();

    // data\movies\<folder>\<file>: false (nothing plays) when the disc lacks it or it does not read
    bool start(const std::string &trackFolder, const std::string &file);
    void update();   // this frame: the camera into the app's view, the speech
    void skip();     // 0x9af10: A / START end it (the voice stops)
    void stop();
    void unloadModels();   // the app's shutdown
    bool playing() const
    {
        return playing_;
    }
    // 0x9aa00: the first track's length less 0.1 s has played, or skipped (0x439f78 = 1); from then on the frame holds
    bool finished() const
    {
        return endedAt_ >= 0.f;
    }
    argon::story::SceneLight light() const
    {
        return sequence_.light();
    }   // light 1 (0x96308)

    // the actors this frame: pose them (the scene recorder, before drawing), then each model and its world
    void poseActors();
    struct ActorDraw
    {
        Track *model;
        glm::mat4 world;
    };
    std::vector<ActorDraw> actorDraws() const;
    void drawOverlay(const FeCanvas &canvas) const;   // the bars and the subtitle (0x98c30)

  private:
    struct Actor
    {
        int object = -1;
        int pilot = -1;   // 0x9b170 of its folder: whose speech animates its face
        Track *model = nullptr;
        int boundAnimation = -1;                 // the CMS object its body tracks come from (0xb55e0)
        std::vector<const argon::CmmTrack *> body;  // per bone
        std::vector<const argon::CmmTrack *> face;  // per bone: the speech's face animation (0xb5710), kept after it
        int faceObject = -1;                     // the speech it came from
        float faceSeconds = 0.f;                 // the bones' own clock (flag 0x80, 0xb9ec0)
        int headBone = -1;                       // what another actor's head turns to (0x9996e)
        glm::vec3 headPoint{0.f};                // its world position as last posed (bone +0xc0, 0x997a2)
        bool headPosed = false;
        SequenceActor state;
    };
    struct Speech
    {
        argon::story::SpeechLine line;
        std::shared_ptr<argon::Pcm> voice;
        argon::CmsAnimation face;
        std::u16string subtitle;
    };

    Track *loadModel(const std::string &folder);
    void loadActors();
    void loadSpeech();
    void loadSubtitles();
    void playSpeech(const SequenceSpeech &speech);
    void bindAnimations(Actor &actor);
    std::vector<argon::BoneKeys> boneDrivers(const Actor &actor) const;
    std::vector<argon::BoneAim> headAims(const Actor &actor) const;
    glm::vec3 lookTarget(const Actor &actor) const;
    void notePosedHead(Actor &actor) const;
    float seconds() const;
    void drawBars(const FeCanvas &canvas) const;
    void drawSubtitle(const FeCanvas &canvas) const;

    StoryHost &host_;
    Sources sources_;
    NmsSequence sequence_;
    std::map<std::string, std::unique_ptr<Track>> models_;   // by folder, kept for the next scene
    std::vector<Actor> actors_;
    std::map<int, Speech> speeches_;                         // by speech object
    std::map<int, std::u16string> subtitles_;                // cutscenes.str
    bool playing_ = false;
    double startedAt_ = 0.0;
    float endedAt_ = -1.f;   // the scene's time when it ended (-1: playing)
    int voiceObject_ = -1;   // 0x1b3c84: the speech whose voice was started last
    int shownObject_ = -1;   // 0x1b3c88: the speech whose subtitle shows (-1 none)
    float speechSeconds_ = 0.f;
    int cameraObject_ = -1;
};
