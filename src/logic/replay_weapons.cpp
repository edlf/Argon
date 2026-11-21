// Argon: the replays' weapon entries (logic\replay_weapons.hpp, docs\replays\README.md).
#include "logic/replay_weapons.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "original/event_buffer_records.hpp"

namespace argon::replay
{
namespace
{

using original::EventProjectileState;
using original::EventWeaponSignal;

constexpr size_t kKind = offsetof(EventWeaponSignal, weapon.kind), kProjectile = offsetof(EventWeaponSignal, weapon.projectile);
constexpr size_t kSlot = offsetof(EventWeaponSignal, weapon.slot), kCraft = offsetof(EventWeaponSignal, weapon.craft);
constexpr size_t kExtra = offsetof(EventWeaponSignal, weapon.extra), kSignal = offsetof(EventWeaponSignal, signal);
constexpr size_t kSignalValue = offsetof(EventWeaponSignal, value), kSignalPosition = offsetof(EventWeaponSignal, where);
constexpr size_t kSignalSecond = kSignalPosition + sizeof(float);   // the pickup's craft (signal 8)
constexpr size_t kStatePosition = offsetof(EventProjectileState, position), kStateVelocity = offsetof(EventProjectileState, velocity);
constexpr size_t kStateForward = offsetof(EventProjectileState, negatedForward);
constexpr float kDirectionScale = 127.f;   // 0x1a3a0c: x 1 / 127 on reading

template <class T> void put(WeaponEvent &event, size_t at, T value)
{
    std::memcpy(event.raw.data() + at, &value, sizeof(T));
}

template <class T> T get(const WeaponEvent &event, size_t at)
{
    T value;
    std::memcpy(&value, event.raw.data() + at, sizeof(T));
    return value;
}

void putVec3(WeaponEvent &event, size_t at, const glm::vec3 &v)
{
    put(event, at, v.x);
    put(event, at + 4, v.y);
    put(event, at + 8, v.z);
}

glm::vec3 getVec3(const WeaponEvent &event, size_t at)
{
    return {get<float>(event, at), get<float>(event, at + 4), get<float>(event, at + 8)};
}

uint8_t byteOf(int value)
{
    return uint8_t(std::clamp(value, 0, 0xff));
}

// the fields every weapon entry has, as the decoded copy and the raw bytes
WeaponEvent entryFor(uint32_t kind, const ProjectileId &id, int extra)
{
    WeaponEvent event;
    event.kind = kind;
    event.projectile = byteOf(id.projectile);
    event.weapon = byteOf(id.slot);
    event.craft = byteOf(id.craft);
    event.extra = byteOf(extra);
    put(event, kKind, kind);
    put(event, kProjectile, uint8_t(event.projectile));
    put(event, kSlot, uint8_t(event.weapon));
    put(event, kCraft, uint8_t(event.craft));
    put(event, kExtra, uint8_t(event.extra));
    return event;
}

WeaponEvent signalFor(Signal signal, const ProjectileId &id, int extra)
{
    WeaponEvent event = entryFor(WeaponEvent::KindSignal, id, extra);
    event.signal = signal;
    put(event, kSignal, uint32_t(signal));
    return event;
}

int8_t directionByte(float v)
{
    return int8_t(std::clamp(std::lround(-v * kDirectionScale), -127l, 127l));
}

}  // namespace

WeaponEvent launchEvent(const ProjectileId &id, int target)
{
    return signalFor(SignalLaunch, id, target < 0 ? kNoTarget : target);
}

WeaponEvent doneEvent(const ProjectileId &id)
{
    return signalFor(SignalDone, id, 0);
}

WeaponEvent impactEvent(const ProjectileId &id, int impactKind, const glm::vec3 &at)
{
    WeaponEvent event = signalFor(SignalImpact, id, id.craft);
    put(event, kSignalValue, int32_t(impactKind));
    putVec3(event, kSignalPosition, at);
    return event;
}

WeaponEvent shieldEvent(int craft, float shield, int killer)
{
    WeaponEvent event = signalFor(SignalShield, ProjectileId{craft, 0, 0}, killer);
    put(event, kSignalPosition, shield);
    return event;
}

WeaponEvent fireEvent(int craft, int slot)
{
    WeaponEvent event = signalFor(SignalFire, ProjectileId{craft, 0, 0}, 0);
    put(event, kSignalValue, int32_t(slot));
    return event;
}

WeaponEvent projectileStateEvent(const ProjectileId &id, const glm::vec3 &pos, const glm::vec3 &vel, const glm::vec3 &forward)
{
    WeaponEvent event = entryFor(WeaponEvent::KindProjectile, id, id.craft);
    putVec3(event, kStatePosition, pos);
    putVec3(event, kStateVelocity, vel);
    put(event, kStateForward, directionByte(forward.x));
    put(event, kStateForward + 1, directionByte(forward.y));
    put(event, kStateForward + 2, directionByte(forward.z));
    return event;
}

WeaponEvent pickupTakenEvent(int pickup, int craft)
{
    WeaponEvent event = signalFor(SignalPickupTaken, ProjectileId{}, 0);
    put(event, kSignalPosition, int32_t(pickup));
    put(event, kSignalSecond, int32_t(craft));
    return event;
}

WeaponEvent pickupBackEvent(int pickup)
{
    WeaponEvent event = signalFor(SignalPickupBack, ProjectileId{}, 0);
    put(event, kSignalPosition, int32_t(pickup));
    return event;
}

ProjectileId projectileOf(const WeaponEvent &event)
{
    return {event.craft, event.weapon, event.projectile};
}

bool isSignal(const WeaponEvent &event, Signal signal)
{
    return event.kind == WeaponEvent::KindSignal && event.signal == signal;
}

int launchTargetOf(const WeaponEvent &event)
{
    return event.extra == kNoTarget ? -1 : event.extra;
}

bool isImpact(const WeaponEvent &event)
{
    return isSignal(event, SignalImpact) || isSignal(event, SignalImpactAfter);
}

float shieldOf(const WeaponEvent &event)
{
    return get<float>(event, kSignalPosition);
}

int killerOf(const WeaponEvent &event)
{
    return event.extra;
}

int impactKindOf(const WeaponEvent &event)
{
    return get<int32_t>(event, kSignalValue);
}

int fireSlotOf(const WeaponEvent &event)
{
    return get<int32_t>(event, kSignalValue);
}

glm::vec3 impactPositionOf(const WeaponEvent &event)
{
    return getVec3(event, kSignalPosition);
}

glm::vec3 statePositionOf(const WeaponEvent &event)
{
    return getVec3(event, kStatePosition);
}

glm::vec3 stateVelocityOf(const WeaponEvent &event)
{
    return getVec3(event, kStateVelocity);
}

glm::vec3 stateForwardOf(const WeaponEvent &event)
{
    const glm::vec3 bytes(float(get<int8_t>(event, kStateForward)), float(get<int8_t>(event, kStateForward + 1)), float(get<int8_t>(event, kStateForward + 2)));
    return -bytes / kDirectionScale;
}

int pickupOf(const WeaponEvent &event)
{
    return get<int32_t>(event, kSignalPosition);
}

int pickupCraftOf(const WeaponEvent &event)
{
    return get<int32_t>(event, kSignalSecond);
}

void ShotTally::count(const WeaponEvent &event)
{
    if (event.kind == WeaponEvent::KindProjectile)
    {
        ++states;
        return;
    }
    fires += isSignal(event, SignalFire) ? 1 : 0;
    launches += isSignal(event, SignalLaunch) ? 1 : 0;
    done += isSignal(event, SignalDone) ? 1 : 0;
    impacts += isImpact(event) ? 1 : 0;
    shields += isSignal(event, SignalShield) ? 1 : 0;
    pickups += isSignal(event, SignalPickupTaken) ? 1 : 0;
    uint32_t value = 0;
    std::memcpy(&value, event.raw.data() + kSignalValue, 4);
    for (const uint64_t v : {uint64_t(event.signal), uint64_t(event.craft), uint64_t(event.weapon), uint64_t(event.projectile), uint64_t(value)})
    {
        sequence = (sequence ^ v) * 1099511628211ull;
    }
}

std::string ShotTally::summary() const
{
    char text[200];
    std::snprintf(text,
                  sizeof text,
                  "%d fires, %d launches, %d done, %d impacts, %d shield values, %d projectile states, %d pickups (sequence %016llx)",
                  fires,
                  launches,
                  done,
                  impacts,
                  shields,
                  states,
                  pickups,
                  static_cast<unsigned long long>(sequence));
    return text;
}

}  // namespace argon::replay
