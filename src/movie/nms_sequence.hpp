// An NMS sequence, one of the game's movies (docs\sequences\README.md; the file's layout: original\nms_records.hpp): objects with
// animation curves, the actors' embedded CMS animations, and the dopesheet's tracks: the "Cameras" track cutting
// between the camera objects, an animation track per actor, the speech track.
#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include <glm/glm.hpp>

#include "model/cms_animation.hpp"
#include "movie/scene_light.hpp"

struct SequenceCamera
{
    int object = -1;      // the camera object (a new one starts a scene transition)
    glm::vec3 eye{0.f}, look{0.f, 0.f, 1.f};
    float fovDeg = 60.f;
};

// a model object (type 2 actor / 10 model) as the actor tracks left it (0x972f0 kind 2): the game keeps the last
// segment's animation while no segment has one
struct SequenceActor
{
    int object = -1;
    glm::mat4 world{1.f};   // the world matrix the model draws with (0x1efd80 + resource x 0x40)
    int animation = -1;     // the CMS object driving it (-1: none yet, the model's own clock, 0x99633)
    float seconds = 0.f;    // the animation's time (0x1ef98c)
    bool loops = false;     // segment flag 1 (0x1efa00); else played once (bone flag 0x400, 0xba000)
    // segment flag 2 (0x972f0): the head turns to an object (0x1f0264, -1 none) by a share (0x1f0288), its world
    // translation (+0x320) the point, an actor's (type 2) head bone instead (0x99908)
    int lookAt = -1;
    float lookShare = 0.f;
    glm::vec3 lookPoint{0.f};
    bool looksAtActor = false;
};

// the speech track's segment now (0x972f0 kind 3): the speech object, or -1 when a group or nothing is there
struct SequenceSpeech
{
    int object = -1;
    float seconds = 0.f;   // the time into its segment (0x439f7c)
};

struct SequenceFrame
{
    std::optional<SequenceCamera> camera;
    std::vector<SequenceActor> actors;   // every model object, in object order
    SequenceSpeech speech;
};

// what a scene loads for one of its objects (0x96230: type 2 / 10 characters\<file>, 9 a speech line by key)
struct SequenceObject
{
    int type = 0;
    std::string name, fileName;
    int resource = -1;
};

class NmsSequence
{
  public:
    static constexpr int kModel = 2, kCameraInterest = 5, kCameraPosition = 6, kAnimation = 8, kSpeech = 9, kShipModel = 10;

    bool load(const std::vector<uint8_t> &file);   // false: not a playable sequence
    std::optional<SequenceCamera> cameraAt(float seconds);
    SequenceFrame frameAt(float seconds);
    float length() const
    {
        return length_;
    }   // the first track's: the scene's time wraps there (0x98b70), a story scene ends there (0x9aa00)
    std::vector<SequenceObject> objects() const;
    const argon::CmsAnimation *animation(int object) const;   // a type-8 object's embedded animation
    argon::story::SceneLight light() const
    {
        return light_;
    }   // the header's light 1 (0x96308)

  private:
    struct Key
    {
        float time = 0, value = 0, weight = 1;
    };
    struct Object
    {
        int type = 0, parent = -1, target = -1, resource = -1;
        std::string name, fileName;
        float values[11]{};                 // +0x370: scale xyz, rotation xyz, translation xyz, (unused), fov
        std::vector<Key> curve[10];
        float maxT = 0.1f;             // +0x3ac
        float curveTime = 0.f;         // +0x3b0: the time its curves were last taken at
        int animationSegment = -1;     // +0x3f0: a model's CMS object this frame
        int cms = -1;                  // a type 8 object's index into animations_
    };
    struct Segment
    {
        float t0 = 0, t1 = 0;
        int object = -1;
        float s0 = 0, s1 = 0;
        uint32_t flags = 0;
        int lookAt = -1;
        float lookShare = 0.f;
    };
    struct Track
    {
        int kind = 0, object = -1;
        float length = 0.f;
        std::vector<Segment> segments;
    };
    struct SegmentTime
    {
        int object = -1;
        float seconds = 0.f;
        uint32_t flags = 0;
        int lookAt = -1;
        float lookShare = 0.f;
    };

    // reading (movie\nms_reader.cpp)
    std::vector<uint16_t> readObjects(const std::vector<uint8_t> &d, int count);   // per object: its stored curves
    bool readAnimations(const std::vector<uint8_t> &bytes, size_t &at);
    bool readCurves(const std::vector<uint8_t> &bytes, size_t &at, const std::vector<uint16_t> &stored);
    bool readCurve(const std::vector<uint8_t> &bytes, size_t &at, std::vector<Key> &curve);
    void computeLengths();
    bool readTracks(const std::vector<uint8_t> &bytes, size_t at);
    void startActors();

    // playing
    SegmentTime segmentAt(const Track &track, float seconds) const;
    void applyCameraTrack(const Track &track, float t);
    void applyActorTrack(const Track &track, float seconds);
    void aimHeads();
    SequenceSpeech speechAt(const Track &track, float t) const;
    void evaluateCamera(int camera, float localTime);
    glm::mat4 localMatrix(int object) const;
    glm::mat4 worldMatrix(int object) const;
    std::optional<SequenceCamera> currentCamera() const;
    static float curveValue(const std::vector<Key> &keys, float seconds);
    static float bezierSegment(const std::vector<Key> &keys, size_t index, float seconds);

    std::vector<Object> objects_;
    std::vector<argon::CmsAnimation> animations_;
    std::vector<Track> tracks_;
    std::vector<SequenceActor> actors_;
    int cameraObject_ = -1;
    float length_ = 0.f;
    argon::story::SceneLight light_;
};
