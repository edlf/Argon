// Argon: Time Attack's ghosts (docs\frontend\game-modes.md "Time Attack"): a lap of the player's craft, recorded each
// race step and played back by its lap time. The game (0x429c0) writes the craft's state (position, velocity,
// orientation, angular velocity, the control bits) to the lap's event buffer every frame and places a ghost craft
// from the last event by dead reckoning (0xd06f0); the viewer keeps every 60 Hz step and blends between two.
#pragma once
#include <cstdint>
#include <cstring>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace ghost
{

struct Sample
{
    float seconds = 0;        // into the lap
    glm::vec3 pos{0.f};
    glm::quat rot{1.f, 0.f, 0.f, 0.f};
    float jet = 0;            // the jet flame's size (the throttle / turbo look)
};

struct Lap
{
    std::vector<Sample> samples;   // in time order
    float time = 0;                // the lap's time; 0 while it is still being recorded
    int pilot = -1;                // the game's character id (0x52cdc0)
    int track = -1;                // the game's track id (0x52ccf8: 0..7, + 8 second route, 0x10 / 0x11 Mars)
    int speed = 0;                 // the speed class it was driven at
    bool valid() const
    {
        return time > 0.f && !samples.empty();
    }
};

// the lap at t seconds in: blended between the two samples around t (before the first: the first). False past the
// lap's last sample, where the game's ghost is hidden (0x429c0: the event buffer runs out, +0x1129 = 1)
inline bool sampleAt(const Lap &lap, float seconds, Sample &out)
{
    const std::vector<Sample> &samples = lap.samples;
    if (samples.empty() || seconds > samples.back().seconds)
    {
        return false;
    }
    if (seconds <= samples.front().seconds)
    {
        out = samples.front();
        return true;
    }
    size_t lo = 0, hi = samples.size() - 1;   // s[lo].t < t <= s[hi].t
    while (hi - lo > 1)
    {
        size_t mid = (lo + hi) / 2;
        (samples[mid].seconds < seconds ? lo : hi) = mid;
    }
    const Sample &before = samples[lo];
    const Sample &after = samples[hi];
    float span = after.seconds - before.seconds, f = span > 0.f ? (seconds - before.seconds) / span : 1.f;
    out.seconds = seconds;
    out.pos = glm::mix(before.pos, after.pos, f);
    out.rot = glm::slerp(before.rot, after.rot, f);
    out.jet = before.jet + (after.jet - before.jet) * f;
    return true;
}

// 0x429c0 at the lap line: the lap just finished becomes the session ghost when there is none yet or it is faster
// (+0xd414 < +0xd418, copied by 0xd9c90)
inline bool replaces(const Lap &finished, const Lap &best)
{
    return finished.valid() && (!best.valid() || finished.time < best.time);
}

// the viewer's ghost file (<game>\ghosts\<track>_<pilot>.ghost; the game keeps its ghosts on the save device): "QRGH",
// u32 version 1, i32 track, pilot, speed, f32 lap time, u32 sample count, then per sample 9 f32: t, position xyz,
// rotation wxyz, jet. Little-endian, as written
// the file's bytes, appended as they are in memory (little-endian on both targets)
class ByteWriter
{
  public:
    void put(const void *data, size_t size)
    {
        const uint8_t *from = static_cast<const uint8_t *>(data);
        bytes_.insert(bytes_.end(), from, from + size);
    }
    template <typename Value> void value(Value field)
    {
        put(&field, sizeof field);
    }
    std::vector<uint8_t> take()
    {
        return std::move(bytes_);
    }

  private:
    std::vector<uint8_t> bytes_;
};

class ByteReader
{
  public:
    explicit ByteReader(const std::vector<uint8_t> &bytes) : bytes_(bytes)
    {
    }
    bool get(void *data, size_t size)   // false (nothing read) past the end
    {
        if (at_ + size > bytes_.size())
        {
            return false;
        }
        std::memcpy(data, bytes_.data() + at_, size);
        at_ += size;
        return true;
    }
    size_t left() const
    {
        return bytes_.size() - at_;
    }

  private:
    const std::vector<uint8_t> &bytes_;
    size_t at_ = 0;
};

inline std::vector<uint8_t> encode(const Lap &lap)
{
    ByteWriter out;
    out.put("QRGH", 4);
    out.value(uint32_t(1));
    out.value(int32_t(lap.track));
    out.value(int32_t(lap.pilot));
    out.value(int32_t(lap.speed));
    out.value(float(lap.time));
    out.value(uint32_t(lap.samples.size()));
    for (const Sample &sample : lap.samples)
    {
        const float values[9] = {sample.seconds, sample.pos.x, sample.pos.y, sample.pos.z, sample.rot.w, sample.rot.x, sample.rot.y, sample.rot.z, sample.jet};
        out.put(values, sizeof values);
    }
    return out.take();
}

// false (lap untouched) for a short, foreign or out-of-order file
inline bool decode(const std::vector<uint8_t> &bytes, Lap &lap)
{
    ByteReader in(bytes);
    char magic[4];
    uint32_t version = 0, count = 0;
    Lap ghostLap;
    int32_t track = 0, pilot = 0, speed = 0;
    if (!in.get(magic, 4) || std::memcmp(magic, "QRGH", 4) != 0 || !in.get(&version, 4) || version != 1)
    {
        return false;
    }
    if (!in.get(&track, 4) || !in.get(&pilot, 4) || !in.get(&speed, 4) || !in.get(&ghostLap.time, 4) || !in.get(&count, 4))
    {
        return false;
    }
    if (count > 1u << 20 || in.left() != size_t(count) * 36)
    {
        return false;
    }
    ghostLap.track = track;
    ghostLap.pilot = pilot;
    ghostLap.speed = speed;
    ghostLap.samples.resize(count);
    for (Sample &sample : ghostLap.samples)
    {
        float v[9];
        in.get(v, sizeof v);
        sample.seconds = v[0];
        sample.pos = glm::vec3(v[1], v[2], v[3]);
        sample.rot = glm::quat(v[4], v[5], v[6], v[7]);
        sample.jet = v[8];
    }
    for (size_t i = 1; i < ghostLap.samples.size(); ++i)
    {
        if (ghostLap.samples[i].seconds < ghostLap.samples[i - 1].seconds)
        {
            return false;
        }
    }
    lap = std::move(ghostLap);
    return true;
}

}  // namespace ghost
