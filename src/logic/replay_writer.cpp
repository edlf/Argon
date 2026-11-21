// Argon: recording a race in the game's replay format (logic\replay_writer.hpp, docs\replays\README.md).
#include "logic/replay_writer.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>

#include "original/event_buffer_records.hpp"

namespace argon::replay
{
namespace
{

using original::EventChunkHeader;
using original::EventCraftEntry;
using original::EventEntryHeader;
using original::EventIndexEntry;
using original::EventWeaponSignal;

class ByteWriter
{
  public:
    explicit ByteWriter(std::vector<uint8_t> &out) : out_(out)
    {
    }
    template <class T> void put(T value)
    {
        const size_t at = out_.size();
        out_.resize(at + sizeof(T));
        std::memcpy(out_.data() + at, &value, sizeof(T));
    }

  private:
    std::vector<uint8_t> &out_;
};

// 0x582d0: rows 0 and 1 (right, up of the row-major matrix) x 4096 as shorts, each in the low half of a dword
void packRotation(const glm::mat3 &rotation, int32_t (&packedRows)[2][3])
{
    for (int row = 0; row < 2; ++row)
    {
        for (int column = 0; column < 3; ++column)
        {
            const float element = std::clamp(rotation[column][row], -1.f, 1.f);
            packedRows[row][column] = int32_t(int16_t(std::lround(element / original::kEventOrientationScale))) & 0xffff;
        }
    }
}

void copyVec3(const glm::vec3 &v, float (&out)[3])
{
    out[0] = v.x;
    out[1] = v.y;
    out[2] = v.z;
}

// the shared head; "new" stays 0 (set on reading)
EventEntryHeader entryHeader(uint32_t vtable, uint32_t type, uint32_t size, const std::string &target, float time)
{
    EventEntryHeader head{};
    head.staleVtable = vtable;
    head.type = type;
    head.size = size;
    head.targetHash = nameHash(target);
    head.time = time;
    return head;
}

// a craft entry (type 1): the shared head, then the state
void writeCraftEntry(std::vector<uint8_t> &bytes, int craft, const CraftEvent &event)
{
    EventCraftEntry entry{};
    entry.head = entryHeader(original::kCraftEntryVtable, original::kEventContinuous, sizeof entry, "craft" + std::to_string(craft), event.time);
    copyVec3(event.pos, entry.position);
    copyVec3(event.vel, entry.velocity);
    copyVec3(event.angularMomentum, entry.angularMomentum);
    packRotation(event.rotation, entry.rotationRows);
    entry.controls = event.controls;
    ByteWriter(bytes).put(entry);
}

// a weapon entry (type 2): the shared head, then the fields after it as built (logic\replay_weapons.cpp)
void writeWeaponEntry(std::vector<uint8_t> &bytes, const WeaponEvent &event)
{
    const size_t start = bytes.size();
    ByteWriter(bytes).put(entryHeader(original::kWeaponEntryVtable, original::kEventCritical, sizeof(EventWeaponSignal), "WeaponEvents", event.time));
    bytes.insert(bytes.end(), event.raw.begin() + sizeof(EventEntryHeader), event.raw.end());
    std::memcpy(bytes.data() + start + offsetof(EventWeaponSignal, weapon.sequence), &event.sequence, sizeof event.sequence);
}

float largestRotationChange(const glm::mat3 &before, const glm::mat3 &after)
{
    float largest = 0;
    for (int column = 0; column < 3; ++column)
    {
        for (int row = 0; row < 3; ++row)
        {
            largest = std::max(largest, std::abs(before[column][row] - after[column][row]));
        }
    }
    return largest;
}

}  // namespace

bool driftedFrom(const CraftEvent &last, const CraftEvent &now)
{
    const glm::vec3 reckoned = last.pos + last.vel * (now.time - last.time);
    return glm::length(reckoned - now.pos) > kPositionDrift || glm::length(now.vel - last.vel) > kVelocityChange ||
           largestRotationChange(last.rotation, now.rotation) > kRotationDrift;
}

uint32_t controlBitsOf(const std::array<float, 13> &slots, bool firing)
{
    uint32_t bits = slots[0] > 50.f ? ControlThrottle : 0u;
    bits |= firing ? ControlFire : 0u;
    const std::pair<int, Control> held[] = {
        {5, ControlLeft}, {6, ControlRight}, {2, ControlDive}, {3, ControlClimb}, {9, ControlBrake}, {7, ControlAirbrakeLeft}, {8, ControlAirbrakeRight}};
    for (const auto &[slot, bit] : held)
    {
        bits |= slots[size_t(slot)] > 0.f ? uint32_t(bit) : 0u;
    }
    return bits;
}

std::array<float, 13> controlSlotsOf(uint32_t bits)
{
    std::array<float, 13> slots{};
    const std::pair<int, Control> held[] = {{0, ControlThrottle},
                                            {5, ControlLeft},
                                            {6, ControlRight},
                                            {2, ControlDive},
                                            {3, ControlClimb},
                                            {9, ControlBrake},
                                            {7, ControlAirbrakeLeft},
                                            {8, ControlAirbrakeRight}};
    for (const auto &[slot, bit] : held)
    {
        slots[size_t(slot)] = (bits & uint32_t(bit)) ? 100.f : 0.f;
    }
    return slots;
}

std::string formatInfo(const Info &lineup)
{
    std::string text = "track " + std::string(trackName(lineup.track)) + "\r\n";
    for (int craft = 0; craft < kMaxCrafts; ++craft)
    {
        if (lineup.pilots[size_t(craft)] >= 0)
        {
            text += "craft" + std::to_string(craft) + " " + std::string(pilotName(lineup.pilots[size_t(craft)])) + "\r\n";
        }
    }
    return text;
}

void RecordingWriter::start(const Info &lineup, float time)
{
    lineup_ = lineup;
    recording_ = true;
    chunks_.clear();
    open_ = Chunk{time, {}};
    hasLast_.fill(false);
    pendingWeapons_.clear();
    weaponCount_ = 0;
    recordedAny_ = false;
}

void RecordingWriter::recordWeapon(const WeaponEvent &event)
{
    if (!recording_)
    {
        return;
    }
    pendingWeapons_.push_back(event);
    pendingWeapons_.back().sequence = weaponCount_++;
}

void RecordingWriter::takePendingWeapons(float time)
{
    for (WeaponEvent &event : pendingWeapons_)
    {
        event.time = time;
        open_.weapons.push_back(event);
    }
    pendingWeapons_.clear();
}

void RecordingWriter::recordFrame(float time, const std::vector<CraftEvent> &crafts)
{
    if (!recording_)
    {
        return;
    }
    if (due(time))
    {
        writeDriftedCrafts(time, crafts);
    }
    takePendingWeapons(time);
    closeChunk(time);
}

void RecordingWriter::stop()
{
    takePendingWeapons(lastRecordTime_);
    if (recording_ && (!open_.entries.empty() || !open_.weapons.empty()))
    {
        chunks_.push_back(open_);
    }
    open_ = Chunk{};
    recording_ = false;
}

bool RecordingWriter::due(float time) const
{
    return !recordedAny_ || time - lastRecordTime_ >= kMinRecordInterval;
}

void RecordingWriter::writeDriftedCrafts(float time, const std::vector<CraftEvent> &crafts)
{
    lastRecordTime_ = time;
    recordedAny_ = true;
    const size_t count = std::min(crafts.size(), size_t(kMaxCrafts));
    for (size_t craft = 0; craft < count; ++craft)
    {
        CraftEvent now = crafts[craft];
        now.time = time;
        if (hasLast_[craft] && !driftedFrom(last_[craft], now))
        {
            continue;
        }
        open_.entries.emplace_back(int(craft), now);
        last_[craft] = now;
        hasLast_[craft] = true;
    }
}

// eventbuffer_advance (0xdab80): the index entry {the chunk's time, its offset}, then a new chunk with the new time
void RecordingWriter::closeChunk(float time)
{
    chunks_.push_back(open_);
    open_ = Chunk{time, {}};
}

RecordingFiles RecordingWriter::files() const
{
    RecordingFiles files;
    files.lineupText = formatInfo(lineup_);
    ByteWriter index(files.index);
    for (const Chunk &chunk : chunks_)
    {
        index.put(EventIndexEntry{chunk.time, uint32_t(files.events.size())});
        const size_t size = sizeof(EventChunkHeader) + sizeof(EventCraftEntry) * chunk.entries.size() + sizeof(EventWeaponSignal) * chunk.weapons.size();
        ByteWriter(files.events).put(EventChunkHeader{uint32_t(size), chunk.time, uint32_t(chunk.entries.size()), uint32_t(chunk.weapons.size())});
        for (const auto &[craft, state] : chunk.entries)
        {
            writeCraftEntry(files.events, craft, state);
        }
        for (const WeaponEvent &event : chunk.weapons)
        {
            writeWeaponEntry(files.events, event);
        }
    }
    index.put(EventIndexEntry{original::kEventIndexEnd, 0});
    return files;
}

}  // namespace argon::replay
