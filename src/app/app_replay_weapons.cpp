// Argon: the replays' weapon entries on the App side (docs\replays\README.md "Weapon entry"): written while the race
// is recorded (the fires 0x6a390, the launches 0x659eb, the projectiles' states 0x646e0 / 0x64870, the done and impact
// signals 0x649f0, the shields' values, the pickups) and played back by ReplayRace as craft_weapon 0x641f0 does: the
// projectiles are launched and then placed from their recorded states, the impacts only show (0x65f80), the shields
// take their recorded values (0x6ac90, knocking out at 0), the pickups go and come back with theirs. Also the
// replayed crafts' control bits (flaps, jets). The entries themselves: logic\replay_weapons.cpp.
#include "app/app.hpp"

namespace
{
constexpr float kStep = 1.f / 60.f;
}  // namespace

// ---- recording

void App::recordWeaponEvent(const argon::replay::WeaponEvent &event)
{
    if (replayRace.playing())
    {
        return;
    }
    recording.recordWeapon(event);
}

// each race step after the weapons: every craft whose shield changed (hits, leech, recharge, corrosion) gets its value,
// a knocked-out one with its killer
void App::recordShieldValues()
{
    for (int racer = 0; racer < race.racerCount(); ++racer)
    {
        const int slot = race.slotOf(racer);
        const argon::weapons::CraftWeapon &weapon = weapons.craft(slot);
        if (!replayRace.playing())
        {
            recording.recordShield(slot, weapon.energy(), weapon.knockedOut() ? weapon.knockedOutBy() : slot);
        }
    }
}

// ---- playback (craft_weapon 0x641f0; the weapons' side: app\weapon_replays.cpp)

void App::playWeaponEvent(const argon::replay::WeaponEvent &event)
{
    using namespace argon::replay;
    if (event.kind == WeaponEvent::KindProjectile)
    {
        weapons.moveReplayedShot(event);
    }
    else if (isSignal(event, SignalShield))
    {
        weapons.setReplayedShield(event);
    }
    else if (isSignal(event, SignalFire))
    {
        weapons.fireReplayedWeapon(event);
    }
    else if (isSignal(event, SignalLaunch))
    {
        weapons.launchReplayedShot(event);
    }
    else if (isSignal(event, SignalDone))
    {
        weapons.endReplayedShot(event);
    }
    else if (isImpact(event))
    {
        weapons.replayImpact(event);
    }
    else if (isSignal(event, SignalPickupTaken))
    {
        replayPickupTaken(event);
    }
    else if (isSignal(event, SignalPickupBack))
    {
        replayPickupBack(event);
    }
}

void App::replayPickupTaken(const argon::replay::WeaponEvent &event)
{
    takeReplayedPickup(argon::replay::pickupOf(event), argon::replay::pickupCraftOf(event));
}

void App::replayPickupBack(const argon::replay::WeaponEvent &event)
{
    bringReplayedPickupBack(argon::replay::pickupOf(event));
}

// each replayed race step: the trails behind the placed projectiles, the beams on their bones (their hits only show),
// the particles and trail sprites aging
void App::stepReplayedWeapons()
{
    weapons.stepReplayed();
    weaponSprites.age(simClock, 1.f / 60.f);
    stepParticles(sim.position());
}

// craft_replay_inputs 0x61c60: the slots from the recorded bits, then the jet and the flaps as a flying craft's
void App::showReplayedControls(int slot, uint32_t controls)
{
    const Found<argon::GameCraft> craft = race.craftInSlot(slot);
    if (!craft)
    {
        return;
    }
    craft->setReplayControls(argon::replay::controlSlotsOf(controls));
    exhausts.updateJet(slot, argon::JetDrive{craft->throttle(), false}, kStep);
    exhausts.updateFlaps(slot, *craft, kStep);
}
