// The game's event buffer files (CEventBuffer, eventbuffer.cpp) byte for byte: the replays and Time Attack's ghosts.
// <name>.index is EventIndexEntry records up to the first time kEventIndexEnd; <name>.events holds per index entry a
// chunk at its offset: an EventChunkHeader, then its entries back to back, each starting with an EventEntryHeader.
// Name prefixes as in cmm_records.hpp. Field meanings: docs\replays\README.md.
#pragma once
#include <cstddef>
#include <cstdint>

namespace argon::original
{

constexpr float kEventIndexEnd = -1.f;
constexpr uint32_t kEventContinuous = 1;   // CEventBuffer::Register: copied into the registered object
constexpr uint32_t kEventCritical = 2;     // CEventBuffer::RegisterCritical: handed to its callback
constexpr uint32_t kCraftEntryVtable = 0x1b6990;    // the craft entry object's vtable; not read back
constexpr uint32_t kWeaponEntryVtable = 0x1b7094;   // the static weapon entry's (0x1be930), as on the disc
constexpr float kEventOrientationScale = 1.f / 4096.f;   // 0x1a3768 (0x423b0), 4096 = 0x1a3560 (0x582d0)

#pragma pack(push, 1)

struct EventIndexEntry
{
    float time;                   // the race clock of the frame
    uint32_t offset;              // its chunk in <name>.events
};

// FUN_000db090
struct EventChunkHeader
{
    uint32_t size;                // the header included
    float time;                   // = the index time
    uint32_t continuousCount;
    uint32_t criticalCount;
};

struct EventEntryHeader
{
    uint32_t staleVtable;
    uint32_t type;                // kEventContinuous / kEventCritical
    uint32_t size;                // the whole entry
    uint32_t targetHash;          // the target's name hash (obj_send_message 0xb4e00)
    uint8_t runtimeNew;           // set to 1 on reading
    uint8_t unused11[3];
    float time;
};

// type 1, registered as "craft%d": the entry object at craft +0x7e4
struct EventCraftEntry
{
    EventEntryHeader head;
    float position[3];            // craft +0x27c
    float velocity[3];            // m/s
    float angularMomentum[3];
    int32_t rotationRows[2][3];   // rows 0 and 1 (right, up) x 4096 as shorts in the low half (0x582d0); row 2 = row 0 x
                                  // row 1 on reading (0x423b0)
    uint32_t controls;            // the control bits (craft_record_inputs 0x61aa0)
};

// the fields every weapon entry has (type 2, "WeaponEvents", played by craft_weapon 0x641f0)
struct EventWeaponFields
{
    uint32_t kind;                // 1 a projectile's state, 2 a signal
    uint8_t projectile;
    uint8_t slot;
    uint8_t craft;
    uint8_t extra;                // the launch's target, the killer, the attacker
    uint32_t sequence;            // a running count (0x439fc8)
};

// kind 1
struct EventProjectileState
{
    EventEntryHeader head;
    EventWeaponFields weapon;
    float position[3];
    float velocity[3];
    int8_t negatedForward[3];     // x 127 (read back x 1/127, 0x1a3a0c); homing: the lock, a colour x 0x10101
    uint8_t unknown3f;
};

// kind 2. where: the impact's position; signal 1 holds the shield (float) in where[0]; signals 8 / 9 hold the pickup
// (i32) in where[0]'s bytes and signal 8 the craft taking it (i32) in where[1]'s
struct EventWeaponSignal
{
    EventEntryHeader head;
    EventWeaponFields weapon;
    uint32_t signal;
    int32_t value;                // the impact kind, the fired slot, the pickups since the last shot
    float where[3];
    uint8_t unused38[8];          // uninitialised memory of the recording build
};

#pragma pack(pop)

static_assert(sizeof(EventIndexEntry) == 8 && sizeof(EventChunkHeader) == 0x10 && sizeof(EventEntryHeader) == 0x18);
static_assert(offsetof(EventEntryHeader, targetHash) == 0xc && offsetof(EventEntryHeader, time) == 0x14);
static_assert(sizeof(EventCraftEntry) == 0x58 && offsetof(EventCraftEntry, velocity) == 0x24);
static_assert(offsetof(EventCraftEntry, angularMomentum) == 0x30 && offsetof(EventCraftEntry, rotationRows) == 0x3c);
static_assert(offsetof(EventCraftEntry, controls) == 0x54 && sizeof(EventWeaponFields) == 0xc);
static_assert(sizeof(EventProjectileState) == 0x40 && offsetof(EventProjectileState, position) == 0x24);
static_assert(offsetof(EventProjectileState, velocity) == 0x30 && offsetof(EventProjectileState, negatedForward) == 0x3c);
static_assert(sizeof(EventWeaponSignal) == 0x40 && offsetof(EventWeaponSignal, weapon.sequence) == 0x20);
static_assert(offsetof(EventWeaponSignal, signal) == 0x24 && offsetof(EventWeaponSignal, where) == 0x2c);

} // namespace argon::original
