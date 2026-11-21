// Argon: recording a race in the game's replay format (CEventBuffer write mode; docs\replays\README.md
// "Recording"). Each recorded frame (0x3eee0) writes an entry for every craft that left the dead reckoning of its last
// entry, then eventbuffer_advance (0xdab80) closes the frame's chunk: so a chunk carries the time of the frame before
// its entries' own. The bytes are what parseRecording (logic\replay.hpp) reads. Pure: no window, GPU or game state.
#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "logic/replay.hpp"

namespace argon::replay
{

constexpr float kMinRecordInterval = 0.015f;   // 0x1a3958: at most one recorded frame per 0.015 s
constexpr float kPositionDrift = 0.1f;         // 0x1a3508: the dead-reckoned position this far off
constexpr float kVelocityChange = 1.0f;        // the velocity changed by this much (m/s)
constexpr float kRotationDrift = 0.05f;        // 0x1a3654 / 0x1a36e4: any rotation element this far off

// 0x3eee0: does a craft's state now need an entry, given its last one? (dead reckoning pos + vel x dt off by more than
// 0.1, the velocity changed by more than 1.0, or a rotation element moved more than 0.05)
bool driftedFrom(const CraftEvent &last, const CraftEvent &now);

// craft_record_inputs 0x61aa0: the control slots (docs\physics\README.md: [0] throttle 0..100, [2] dive, [3] climb, [5] / [6]
// steer, [7] / [8] airbrakes, [9] brake) as the entry's bits
uint32_t controlBitsOf(const std::array<float, 13> &slots, bool firing);
// craft_replay_inputs 0x61c60: the bits back as control slots, 0 or 100
std::array<float, 13> controlSlotsOf(uint32_t bits);

// the three files of a recording: <name>.info, <name>.index, <name>.events
struct RecordingFiles
{
    std::string lineupText;
    std::vector<uint8_t> index, events;
};
std::string formatInfo(const Info &lineup);   // `track <t>` and `craft<i> <pilot>` lines, as the disc's

class RecordingWriter
{
  public:
    // neon_gamelogic_record_replay 0x3f210: the buffer cleared, the crafts registered; the first chunk starts at time
    void start(const Info &lineup, float time);
    // 0x3eee0 then eventbuffer_advance: one frame's craft states (craft i's in [i]) at the race clock
    void recordFrame(float time, const std::vector<CraftEvent> &crafts);
    // CEventBuffer::WriteCriticalEntry 0xdaa60: a weapon entry of this frame (its time and running count set here)
    void recordWeapon(const WeaponEvent &event);
    void stop();   // 0x3f2f0
    bool recording() const
    {
        return recording_;
    }
    size_t frames() const
    {
        return chunks_.size();
    }
    RecordingFiles files() const;   // the chunks closed so far and, after stop, the last one

  private:
    struct Chunk
    {
        float time = 0;
        std::vector<std::pair<int, CraftEvent>> entries;   // craft, its state (its own time)
        std::vector<WeaponEvent> weapons;
    };
    bool due(float time) const;
    void writeDriftedCrafts(float time, const std::vector<CraftEvent> &crafts);
    void closeChunk(float time);
    void takePendingWeapons(float time);

    Info lineup_;
    bool recording_ = false;
    std::vector<Chunk> chunks_;   // closed
    Chunk open_;
    std::array<CraftEvent, kMaxCrafts> last_{};
    std::array<bool, kMaxCrafts> hasLast_{};
    std::vector<WeaponEvent> pendingWeapons_;   // written this frame, before its time is known
    uint32_t weaponCount_ = 0;                  // +0x20 (0x439fc8)
    float lastRecordTime_ = 0;
    bool recordedAny_ = false;
};

}  // namespace argon::replay
