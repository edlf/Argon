// Argon: playing an NMS sequence (movie\nms_sequence.hpp; reading it: movie\nms_reader.cpp). Each frame (seq_update
// 0x98b70) the time wraps at the first track's length and every track takes its segment at that time (0x972f0).
#include "movie/nms_sequence.hpp"

#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

#include "movie/head_turn.hpp"
#include "original/nms_records.hpp"

namespace
{
const int kFovSlot = 9;   // curve slot 9 drives p[10]
const int kCameras = 1, kActorAnimation = 2, kSpeechTrack = 3;   // track kinds (+0x50)
const uint32_t kLoops = 1, kTurnsHead = 2;                       // actor segment flags
const int kGroup = 0;
const float kDegToRad = 3.14159265358979f / 180.f;

// a third of the slope between two keys
template <class Key> float slope(const Key &from, const Key &to)
{
    return (to.value - from.value) / 3.f / (to.time - from.time);
}

// 0x97740: the row-vector matrix S x Rz x Rx x Ry (D3D's rotations) as a glm column-vector matrix
glm::mat4 scaledRotation(const float *values)
{
    const glm::mat4 rx = glm::rotate(glm::mat4(1.f), values[3] * kDegToRad, glm::vec3(1, 0, 0));
    const glm::mat4 ry = glm::rotate(glm::mat4(1.f), values[4] * kDegToRad, glm::vec3(0, 1, 0));
    const glm::mat4 rz = glm::rotate(glm::mat4(1.f), values[5] * kDegToRad, glm::vec3(0, 0, 1));
    const glm::mat4 scale = glm::scale(glm::mat4(1.f), glm::vec3(values[0], values[1], values[2]));
    return ry * rx * rz * scale;
}
} // namespace

std::vector<SequenceObject> NmsSequence::objects() const
{
    std::vector<SequenceObject> list;
    for (const Object &object : objects_)
    {
        list.push_back({object.type, object.name, object.fileName, object.resource});
    }
    return list;
}

const argon::CmsAnimation *NmsSequence::animation(int object) const
{
    if (object < 0 || object >= int(objects_.size()) || objects_[size_t(object)].cms < 0)
    {
        return nullptr;
    }
    return &animations_[size_t(objects_[size_t(object)].cms)];
}

std::optional<SequenceCamera> NmsSequence::cameraAt(float seconds)
{
    return frameAt(seconds).camera;
}

// 0x98b70: t = fmod(the clock, the first track's length); the tracks in file order
SequenceFrame NmsSequence::frameAt(float seconds)
{
    SequenceFrame frame;
    const float clock = std::fmod(seconds, length_);
    for (const Track &track : tracks_)
    {
        if (track.kind == kCameras)
        {
            applyCameraTrack(track, clock);
        }
        if (track.kind == kActorAnimation)
        {
            applyActorTrack(track, clock);
        }
        if (track.kind == kSpeechTrack)
        {
            frame.speech = speechAt(track, clock);
        }
    }
    frame.camera = currentCamera();
    for (SequenceActor &actor : actors_)
    {
        actor.world = worldMatrix(actor.object);
    }
    aimHeads();
    frame.actors = actors_;
    return frame;
}

// 0x971d0: the last of the segments (all but the final one) holding t, else the first; its object's own time
// (t - start) x f + source start x f, f = the object's length / the source span
NmsSequence::SegmentTime NmsSequence::segmentAt(const Track &track, float seconds) const
{
    if (track.segments.empty())
    {
        return {};
    }
    size_t found = 0;
    for (size_t i = 0; i + 1 < track.segments.size(); ++i)
    {
        if (track.segments[i].t0 <= seconds && seconds <= track.segments[i].t1)
        {
            found = i;
        }
    }
    const Segment &segment = track.segments[found];
    if (segment.object < 0 || segment.object >= int(objects_.size()))
    {
        return {};
    }
    const float rate = objects_[size_t(segment.object)].maxT / (segment.s1 - segment.s0);
    return {segment.object, (seconds - segment.t0) * rate + rate * segment.s0, segment.flags, segment.lookAt, segment.lookShare};
}

// 0x972f0 kind 1: the camera and its children take their curves at the cut's time (0x98900)
void NmsSequence::applyCameraTrack(const Track &track, float seconds)
{
    const SegmentTime cut = segmentAt(track, std::clamp(seconds, 0.f, track.length));
    if (cut.object < 0)
    {
        return;
    }
    cameraObject_ = cut.object;
    evaluateCamera(cut.object, cut.seconds);
}

// 0x972f0 kind 2: a segment with an animation gives the actor its animation, time and loop flag; the model's
// world takes the animation object's matrix (0x97e90 +0x3f0)
void NmsSequence::applyActorTrack(const Track &track, float seconds)
{
    if (track.object < 0)
    {
        return;
    }
    const SegmentTime segment = segmentAt(track, std::clamp(seconds, 0.f, track.length));
    Object &model = objects_[size_t(track.object)];
    model.animationSegment = segment.object;
    for (SequenceActor &actor : actors_)
    {
        if (actor.object != track.object)
        {
            continue;
        }
        const bool animated = animation(segment.object) != nullptr;
        const bool turns = animated && (segment.flags & kTurnsHead) != 0;
        actor.lookAt = turns ? segment.lookAt : -1;
        actor.lookShare = turns ? segment.lookShare : 0.f;
        if (animated)
        {
            actor.animation = segment.object;
            actor.seconds = segment.seconds;
            actor.loops = (segment.flags & kLoops) != 0;
        }
    }
}

// 0x99360: a head turns only to an object 0 < object < count (0x998b2); the point is its world translation, or for
// an actor its head bone, which the scene finds once the actor is posed
void NmsSequence::aimHeads()
{
    for (SequenceActor &actor : actors_)
    {
        if (!argon::story::turnsHead(actor.lookAt, int(objects_.size())))
        {
            actor.lookAt = -1;
            continue;
        }
        actor.lookPoint = glm::vec3(worldMatrix(actor.lookAt)[3]);
        actor.looksAtActor = objects_[size_t(actor.lookAt)].type == kModel;
    }
}

// 0x972f0 kind 3 (no clamp): nothing or a group stops the speech (0x96dd0 with -1)
SequenceSpeech NmsSequence::speechAt(const Track &track, float seconds) const
{
    const SegmentTime segment = segmentAt(track, seconds);
    if (segment.object < 0 || objects_[size_t(segment.object)].type == kGroup)
    {
        return {};
    }
    return {segment.object, segment.seconds};
}

// the camera and its children take their curves at that time (0x98900, slots 0..8 = +0x370..+0x390, slot 9 = the fov
// +0x398)
void NmsSequence::evaluateCamera(int camera, float localTime)
{
    for (int i = 0; i < int(objects_.size()); ++i)
    {
        Object &object = objects_[size_t(i)];
        if (i != camera && object.parent != camera)
        {
            continue;
        }
        for (int slot = 0; slot < argon::original::kNmsCurveSlots; ++slot)
        {
            if (!object.curve[slot].empty())
            {
                object.values[slot < kFovSlot ? slot : 10] = curveValue(object.curve[slot], localTime);
            }
        }
    }
}

// 0x99419: eye = the position's world translation, look-at = the interest's, fov = the position's
std::optional<SequenceCamera> NmsSequence::currentCamera() const
{
    if (cameraObject_ < 0)
    {
        return std::nullopt;
    }
    int pos = -1, look = -1;
    for (int i = 0; i < int(objects_.size()); ++i)
    {
        if (objects_[size_t(i)].parent != cameraObject_)
        {
            continue;
        }
        pos = objects_[size_t(i)].type == kCameraPosition ? i : pos;
        look = objects_[size_t(i)].type == kCameraInterest ? i : look;
    }
    if (pos < 0 || look < 0)
    {
        return std::nullopt;
    }
    SequenceCamera camera{cameraObject_, glm::vec3(worldMatrix(pos)[3]), glm::vec3(worldMatrix(look)[3]), objects_[size_t(pos)].values[10]};
    if (glm::length(camera.look - camera.eye) < 1e-4f)
    {
        camera.look = camera.eye + glm::vec3(0, 0, 1);
    }
    return camera;
}

// 0x97d40: scale, rotation (0x97740) and translation (row 3)
glm::mat4 NmsSequence::localMatrix(int object) const
{
    const float *values = objects_[size_t(object)].values;
    glm::mat4 pose = scaledRotation(values);
    pose[3] = glm::vec4(values[6], values[7], values[8], 1.f);
    return pose;
}

// 0x97e90: world = local x parent world; a model with an animation segment has the animation object's local between
// (a camera position's aim at its interest, 0x98030, turns only its own children: the files give it none)
glm::mat4 NmsSequence::worldMatrix(int object) const
{
    glm::mat4 world = localMatrix(object);
    for (int depth = 0; depth < 64 && object >= 0; ++depth)
    {
        const Object &sequenceObject = objects_[size_t(object)];
        const bool animated = (sequenceObject.type == kModel || sequenceObject.type == kShipModel) && sequenceObject.animationSegment >= 0;
        if (animated && sequenceObject.animationSegment < int(objects_.size()))
        {
            world = localMatrix(sequenceObject.animationSegment) * world;
        }
        object = sequenceObject.parent;
        if (object >= 0)
        {
            world = localMatrix(object) * world;
        }
    }
    return world;
}

// 0x98720: before the first key the first value, after the last the last (bounds 0x985e0); between keys i and i + 1 a
// cubic Bezier (0x986c0) whose inner points come from the neighbouring slopes mixed by the keys' weights
float NmsSequence::curveValue(const std::vector<Key> &keys, float seconds)
{
    if (keys.empty())
    {
        return 0.f;
    }
    float lo = 9999999.f, hi = -9999999.f;
    for (auto &key : keys)
    {
        lo = std::min(lo, key.time);
        hi = std::max(hi, key.time);
    }
    if (seconds <= lo)
    {
        return keys.front().value;
    }
    if (seconds >= hi)
    {
        return keys.back().value;
    }
    for (size_t i = 0; i + 1 < keys.size(); ++i)
    {
        if (keys[i].time <= seconds && seconds <= keys[i + 1].time)
        {
            return bezierSegment(keys, i, seconds);
        }
    }
    return keys.front().value;
}

float NmsSequence::bezierSegment(const std::vector<Key> &keys, size_t index, float seconds)
{
    const Key &from = keys[index], &b = keys[index + 1];
    const float dt = b.time - from.time;
    const float in = index > 0 ? slope(keys[index - 1], b) * keys[index - 1].weight + (1.f - keys[index - 1].weight) * slope(keys[index - 1], from) : 0.f;
    const float lin = (1.f - from.weight) * slope(from, b);
    const float out = index + 2 < keys.size() ? slope(from, keys[index + 2]) * b.weight + (1.f - b.weight) * slope(b, keys[index + 2]) : 0.f;
    const float p1 = (in * from.weight + lin) * dt + from.value;
    const float p2 = b.value - (out * from.weight + lin) * dt;
    const float share = (seconds - from.time) / dt, r = 1.f - share;
    return r * r * r * from.value + 3.f * share * r * r * p1 + 3.f * share * share * r * p2 + share * share * share * b.value;
}
