// Argon: reading an NMS sequence (movie\nms_sequence.hpp) in the order seq_start_copy (0x96230) links it: the
// objects, the type-8 objects' CMS animations, every object's stored curves, the dopesheet's tracks.
#include "movie/nms_sequence.hpp"

#include <algorithm>

#include "model/cmm_tracks.hpp"
#include "original/nms_records.hpp"
#include "original/record_read.hpp"

namespace
{
using namespace argon::original;

const int kMaxObjects = 4096;
const int kFovSlot = 9;   // curve slot 9 drives p[10]

// 0x98190 skips these: a dopesheet, an animation and a speech keep the length the file stores
bool isTimeless(int type)
{
    return type == 3 || type == 7 || type == NmsSequence::kAnimation || type == NmsSequence::kSpeech;
}

bool isUnderCamera(int type)
{
    return type == NmsSequence::kCameraPosition || type == NmsSequence::kCameraInterest;
}
} // namespace

// 0x96230
bool NmsSequence::load(const std::vector<uint8_t> &bytes)
{
    objects_.clear();
    animations_.clear();
    tracks_.clear();
    cameraObject_ = -1;
    length_ = 0.f;
    const auto header = readAt<NmsHeader>(bytes, 0);
    const int count = header.objectCount;
    if (count <= 0 || count > kMaxObjects || sizeof(NmsHeader) + size_t(count) * sizeof(NmsObject) > bytes.size())
    {
        return false;
    }
    const std::vector<uint16_t> stored = readObjects(bytes, count);
    size_t at = sizeof(NmsHeader) + size_t(count) * sizeof(NmsObject);
    if (!readAnimations(bytes, at) || !readCurves(bytes, at, stored))
    {
        return false;
    }
    light_ = argon::story::sceneLight(glm::vec3(header.lightDirection[0], header.lightDirection[1], header.lightDirection[2]), header.lightColour);
    computeLengths();
    startActors();
    return readTracks(bytes, at);
}

// the game resets out-of-range links to -1
std::vector<uint16_t> NmsSequence::readObjects(const std::vector<uint8_t> &bytes, int count)
{
    std::vector<uint16_t> stored(static_cast<size_t>(count));
    objects_.resize(size_t(count));
    for (int i = 0; i < count; ++i)
    {
        const auto record = readAt<NmsObject>(bytes, sizeof(NmsHeader) + size_t(i) * sizeof(NmsObject));
        Object &object = objects_[size_t(i)];
        object.type = record.type;
        object.parent = record.parent >= 0 && record.parent < count ? record.parent : -1;
        object.target = record.partner >= 0 && record.partner < count ? record.partner : -1;
        object.resource = record.resourceIndex;
        object.name = argon::fixedString(record.name, sizeof record.name);
        object.fileName = argon::fixedString(record.fileName, sizeof record.fileName);
        std::copy(std::begin(record.scale), std::end(record.scale), object.values);
        std::copy(std::begin(record.rotationDegrees), std::end(record.rotationDegrees), object.values + 3);
        std::copy(std::begin(record.translation), std::end(record.translation), object.values + 6);
        object.values[10] = record.fovDegrees;
        object.maxT = isTimeless(object.type) ? record.runtimeLength : 0.1f;
        object.curveTime = record.runtimeCurveTime;
        for (int slot = 0; slot < kNmsCurveSlots; ++slot)
        {
            if (record.curveStored[slot])
            {
                stored[size_t(i)] |= uint16_t(1u << slot);
            }
        }
    }
    return stored;
}

// 0x96401: each type-8 object's animation follows the objects, in object order (0xb5270 links it, returning its size)
bool NmsSequence::readAnimations(const std::vector<uint8_t> &bytes, size_t &at)
{
    for (Object &object : objects_)
    {
        if (object.type != kAnimation)
        {
            continue;
        }
        argon::CmsAnimation animation;
        const std::optional<size_t> end = argon::readCms(bytes, at, animation);
        if (!end)
        {
            return false;
        }
        object.cms = int(animations_.size());
        animations_.push_back(std::move(animation));
        at = *end;
    }
    return true;
}

bool NmsSequence::readCurves(const std::vector<uint8_t> &bytes, size_t &at, const std::vector<uint16_t> &stored)
{
    for (size_t i = 0; i < objects_.size(); ++i)
    {
        for (int slot = 0; slot < kNmsCurveSlots; ++slot)
        {
            if ((stored[i] >> slot & 1) && !readCurve(bytes, at, objects_[i].curve[slot]))
            {
                return false;
            }
        }
    }
    return true;
}

bool NmsSequence::readCurve(const std::vector<uint8_t> &bytes, size_t &at, std::vector<Key> &curve)
{
    const int count = readAt<NmsCurveHeader>(bytes, at).keyCount;
    const size_t firstKey = at + sizeof(NmsCurveHeader);
    if (count < 0 || firstKey + size_t(count) * sizeof(NmsKey) > bytes.size())
    {
        return false;
    }
    for (int k = 0; k < count; ++k)
    {
        const auto key = readAt<NmsKey>(bytes, firstKey + size_t(k) * sizeof(NmsKey));
        curve.push_back({key.time, key.value, key.weight});
    }
    at = firstKey + size_t(count) * sizeof(NmsKey);
    return true;
}

// 0x98190: an object's length = its last key time (at least 0.1); a camera's position / interest raise their camera's
void NmsSequence::computeLengths()
{
    for (auto &object : objects_)
    {
        if (isTimeless(object.type))
        {
            continue;
        }
        float last = 0.1f;
        for (auto &curve : object.curve)
        {
            for (auto &key : curve)
            {
                last = std::max(last, key.time);
            }
        }
        object.maxT = std::max(object.maxT, last);
        if (isUnderCamera(object.type) && object.parent >= 0)
        {
            objects_[size_t(object.parent)].maxT = std::max(objects_[size_t(object.parent)].maxT, object.maxT);
        }
    }
}

// the objects no camera cut drives keep their curves at the time the file stores (0x98b70 evaluates them there; one
// group in all the files has curves); the models start without an animation (0x9a020: 0x1effa4 = -1)
void NmsSequence::startActors()
{
    actors_.clear();
    for (int i = 0; i < int(objects_.size()); ++i)
    {
        Object &object = objects_[size_t(i)];
        if (object.type != 4 && !isUnderCamera(object.type))
        {
            for (int slot = 0; slot < kNmsCurveSlots; ++slot)
            {
                if (!object.curve[slot].empty())
                {
                    object.values[slot < kFovSlot ? slot : 10] = curveValue(object.curve[slot], object.curveTime);
                }
            }
        }
        if (object.type == kModel || object.type == kShipModel)
        {
            SequenceActor actor;
            actor.object = i;
            actors_.push_back(actor);
        }
    }
}

// the tracks after the curves (0x9663b): a count, then per track 4 bytes, the record and its segments; the first
// track's length is the sequence's
bool NmsSequence::readTracks(const std::vector<uint8_t> &bytes, size_t at)
{
    const int tracks = readAt<int32_t>(bytes, at);
    at += sizeof(int32_t);
    for (int t = 0; t < tracks && at + sizeof(NmsTrack) <= bytes.size(); ++t)
    {
        const auto record = readAt<NmsTrack>(bytes, at);
        const size_t firstSegment = at + sizeof(NmsTrack);
        const int count = record.segmentCount;
        if (count < 0 || firstSegment + size_t(count) * sizeof(NmsSegment) > bytes.size())
        {
            return false;
        }
        Track track;
        track.kind = record.kind;
        track.object = record.object >= 0 && record.object < int(objects_.size()) ? record.object : -1;
        track.length = record.length;
        for (int k = 0; k < count; ++k)
        {
            const auto segment = readAt<NmsSegment>(bytes, firstSegment + size_t(k) * sizeof(NmsSegment));
            track.segments.push_back({segment.start, segment.end, segment.object, segment.sourceStart, segment.sourceEnd, segment.flags, segment.lookAt, segment.lookStrength});
        }
        tracks_.push_back(std::move(track));
        at = firstSegment + size_t(count) * sizeof(NmsSegment);
    }
    if (tracks_.empty())
    {
        return false;
    }
    length_ = tracks_.front().length;
    return length_ > 0.f;
}
