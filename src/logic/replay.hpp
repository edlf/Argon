// Argon: the game's replays, its event buffer files (CEventBuffer, eventbuffer.cpp; docs\replays\README.md). A
// replay is three files: <name>.info (the track and the crafts' pilots, text), <name>.index (a time and an offset into
// the events per recorded frame) and <name>.events (per frame a chunk of entries: the crafts' states, recorded when
// they leave the dead reckoning of the last one, and the weapons' events). The attract demo plays d:\replays\replay1..3.
// Pure: no window, GPU or game state.
#pragma once
#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace argon::replay
{

constexpr int kMaxCrafts = 16;

// obj_send_message 0xb4e00: the id an entry carries for the object it is addressed to ("craft0".., "WeaponEvents")
uint32_t nameHash(std::string_view name);

// <name>.info, read by fe_shell_start_attract 0x31c10: `track <t>` and `craft<i> <pilot>` lines
struct Info
{
    int track = -1;                       // the game's track id (0x52ccf8)
    std::array<int, kMaxCrafts> pilots;   // the game's character id per craft (0x52cdc0), -1 for none
    Info()
    {
        pilots.fill(-1);
    }
    int craftCount() const;
};
Info parseInfo(std::string_view text);
int trackIdNamed(std::string_view name);   // "silver" 0 .. "alt_mars" 0x11; -1 for an unknown name
int pilotIdNamed(std::string_view name);   // "tara" 0 .. "kutoo" 15; -1 for an unknown name
std::string_view trackName(int id);         // the .info name of a track id; empty for an unknown id
std::string_view pilotName(int id);         // the .info name of a character id; empty for an unknown id

// <name>.index: one entry per recorded frame, up to the first time -1
struct IndexEntry
{
    float time = 0;        // the race clock of the frame
    uint32_t offset = 0;   // its chunk in <name>.events
};
std::vector<IndexEntry> parseIndex(const std::vector<uint8_t> &bytes);

// the control bits of a craft entry (+0x838, craft_record_inputs 0x61aa0 / craft_replay_inputs 0x61c60)
enum Control : uint32_t
{
    ControlThrottle = 1,         // slot [0] above 50
    ControlFire = 2,             // +0x91f
    ControlLeft = 4,             // [5]
    ControlRight = 8,            // [6]
    ControlDive = 0x10,          // [2]
    ControlClimb = 0x20,         // [3]
    ControlBrake = 0x40,         // [9]
    ControlAirbrakeLeft = 0x80,  // [7]
    ControlAirbrakeRight = 0x100 // [8]
};

// a craft's state (entry type 1, 0x58 bytes)
struct CraftEvent
{
    float time = 0;                   // the frame it is played on (the game replaces the entry's own with its index time)
    glm::vec3 pos{0.f};               // +0x18
    glm::vec3 vel{0.f};               // +0x24, m/s
    glm::vec3 angularMomentum{0.f};   // +0x30 (the state block's L)
    glm::mat3 rotation{1.f};          // +0x3c: the first two rows x 4096 as shorts, the third their cross product
    uint32_t controls = 0;            // +0x54, Control bits
};

// a weapon event (entry type 2 "WeaponEvents", 0x40 bytes; played by craft_weapon 0x641f0)
struct WeaponEvent
{
    enum Kind : uint32_t
    {
        KindProjectile = 1,   // a projectile's position and velocity
        KindSignal = 2        // a signal: launch, impact, pickup taken, ...
    };
    float time = 0;
    uint32_t kind = 0;
    int projectile = 0, weapon = 0, craft = 0, extra = 0;   // +0x1c .. +0x1f
    uint32_t sequence = 0;                                  // +0x20, counts the recorded weapon events
    uint32_t signal = 0;                                    // +0x24 for KindSignal
    std::array<uint8_t, 0x40> raw{};                        // the entry as recorded
};

struct Recording
{
    Info lineup;
    std::vector<IndexEntry> index;
    std::array<std::vector<CraftEvent>, kMaxCrafts> crafts;   // per craft, in time order
    std::vector<WeaponEvent> weapons;                         // in time order
    float endTime() const
    {
        return index.empty() ? 0.f : index.back().time;
    }
};

// throws std::runtime_error when a chunk or an entry does not fit the files
Recording parseRecording(std::string_view lineupText, const std::vector<uint8_t> &index, const std::vector<uint8_t> &events);
// <folder>\<stem>.info / .index / .events; throws std::runtime_error when a file is missing
Recording loadRecording(const std::filesystem::path &folder, const std::string &stem);

// a craft's pose at a time: between the two entries around it (the game dead-reckons from the last one, 0xd06f0)
struct Pose
{
    glm::vec3 pos{0.f};
    glm::quat rot{1.f, 0.f, 0.f, 0.f};
    glm::vec3 vel{0.f};
    uint32_t controls = 0;
    glm::mat4 matrix() const;   // columns right, up, forward, position (as GameCraft::matrix)
};
// false before the craft's first entry (the game marks the craft dead until one arrives, +0x1145)
bool poseAt(const std::vector<CraftEvent> &events, float seconds, Pose &out);

// craft_replay_inputs 0x61c60: while the clock is under 3 s (0x1a351c) a replayed craft moves 10 % (0x1a3508) of the
// way from where it is to its recorded position each frame; after that it takes the recorded position
constexpr float kEaseSeconds = 3.f, kEaseShare = 0.1f;
glm::vec3 easedStartPosition(const glm::vec3 &current, const glm::vec3 &recorded, float clock);

}  // namespace argon::replay
