// Argon: the race's event sounds, see race_sounds.hpp.
#include "app/race_sounds.hpp"

#include <algorithm>

#include "logic/race.hpp"

namespace
{
constexpr int kIncomingFrames = 20;   // 0xc2f90: a target's warning holds 20 frames after its last call
constexpr int kProjectileClass = 0, kBeamClass = 1, kShieldClass = 2;   // weapon.nwp record +8
constexpr int kCrafts = 16;

bool isCraft(int slot)
{
    return slot >= 0 && slot < kCrafts;
}
}  // namespace

// each 60 Hz step after the crafts moved (0xc4bf0 -> 0xc2b50)
void RaceSounds::stepCrafts(const Race &race, const RaceState &state)
{
    sfx_.setCentred(state.humanPlayers > 1);
    for (int racer = 0; racer < race.racerCount(); ++racer)
    {
        const int slot = race.slotOf(racer);
        const argon::GameCraft &craft = race.racer(racer);
        sfx_.placeCraft(slot, craft.position());
        playContactSounds(slot, craft.soundContacts());
        holdIncomingWarning(slot);
    }
    startDueTravelLoops(state.clock);
}

void RaceSounds::playContactSounds(int slot, const argon::GameCraft::SoundContacts &contacts)
{
    playWorldContact(slot, contacts.world);
    const argon::GenericSound ship = argon::sound::shipImpactSound(contacts.pairChange, impactSettings_);
    playHitSound(ship, slot, argon::sound::hitVolumeMb(contacts.pairChange, impactSettings_.ship, hitVolumeRange(ship)));
    if (contacts.splashed)
    {
        playSplash(slot, contacts.splashSpeed);
    }
}

// 0xc2b50: a scrape loop starts once and holds; a hit ends the craft's scrapes
void RaceSounds::playWorldContact(int slot, const argon::sound::CraftContact &contact)
{
    const argon::sound::ContactSound sound = argon::sound::worldContactSound(contact, impactSettings_);
    if (sound.stopScrapes)
    {
        sfx_.stop(argon::GenericSound::ScrapeMetal, slot);
        sfx_.stop(argon::GenericSound::ScrapeWall, slot);
    }
    const float volume = argon::sound::hitVolumeMb(sound.force, impactSettings_.world, hitVolumeRange(sound.sound));
    if (!sound.scrape)
    {
        playHitSound(sound.sound, slot, volume);
        return;
    }
    if (sound.sound != argon::GenericSound::None && !sfx_.isPlaying(sound.sound, slot))
    {
        sfx_.setVolumeMb(sfx_.playAtCraft(sound.sound, slot), volume);
    }
}

// craft_water 0x61340: not for ghosts (+0x1144; the viewer's ghosts are no crafts)
void RaceSounds::playSplash(int slot, float verticalSpeed)
{
    const argon::sound::Splash splash = argon::sound::waterSplash(verticalSpeed, impactSettings_);
    if (splash.sound == argon::GenericSound::None || sfx_.isPlaying(splash.unlessPlaying, slot))
    {
        return;
    }
    playHitSound(splash.sound, slot, argon::sound::hitVolumeMb(-verticalSpeed, impactSettings_.water, hitVolumeRange(splash.sound)));
}

// 0xc3ad0: a craft's hit sound does not start again within the impact delay
void RaceSounds::playHitSound(argon::GenericSound id, int slot, float volumeMb)
{
    if (id == argon::GenericSound::None || sfx_.startedWithin(id, slot, impactSettings_.impactDelay))
    {
        return;
    }
    sfx_.setVolumeMb(sfx_.playAtCraft(id, slot), volumeMb);
}

argon::sound::VolumeRange RaceSounds::hitVolumeRange(argon::GenericSound id) const
{
    return sfx_.options(id).volume;
}

// 0xc2f90, from a homing projectile or a beam on its target: a human player's warning, restarted every 20 frames
void RaceSounds::playIncoming(int target, const RaceState &state)
{
    if (!isCraft(target) || target >= state.humanPlayers || !state.racing || incomingFrames_[size_t(target)] > 0)
    {
        return;
    }
    incomingFrames_[size_t(target)] = kIncomingFrames;
    sfx_.playAtCraft(argon::GenericSound::Incoming, target);
}

// 0xc4bf0: the count runs down each frame; at 0 the warning stops
void RaceSounds::holdIncomingWarning(int slot)
{
    if (!isCraft(slot))
    {
        return;
    }
    int &frames = incomingFrames_[size_t(slot)];
    if (frames > 0)
    {
        --frames;
        return;
    }
    sfx_.stop(argon::GenericSound::Incoming, slot);
}

// weapon_damage 0x6ae5a: a human player losing shield hears it where the hit landed
void RaceSounds::playPlayerHit(int target, const glm::vec3 &at, const RaceState &state)
{
    if (target >= 0 && target < state.humanPlayers)
    {
        sfx_.playAtPoint(argon::GenericSound::PlayerHit, at);
    }
}

void RaceSounds::playProjectileImpact(const glm::vec3 &at)
{
    sfx_.playAtPoint(argon::GenericSound::ProjectileImpact, at);
    sfx_.playAtPoint(argon::GenericSound::ProjectileImpact, at);
}

void RaceSounds::playMegaImpact(const glm::vec3 &at)
{
    sfx_.playAtPoint(argon::GenericSound::MegaImpact, at);
}

// 0x6908a: the beam on a craft, unless that craft's is still playing; not in the attract demo (0x52afac)
void RaceSounds::playLaserImpact(int target, const RaceState &state)
{
    if (!state.replaying && !sfx_.isPlaying(argon::GenericSound::LaserImpact, target))
    {
        sfx_.playAtCraft(argon::GenericSound::LaserImpact, target);
    }
}

// player_pad_input 0x70d92 for each human player: turbo held with turbo left, held without, let go; not while the
// race is held (0x52af9e). A craft the AI flies (past the finish) runs no pad input, so its turbo is let go: the AI's
// turbo test and the turbo held at the line would otherwise flip the sound between fire and fail every few frames
static argon::RaceSfx::Turbo padTurboOf(const argon::GameCraft &craft)
{
    if (!craft.flownByPad())
    {
        return argon::RaceSfx::Turbo::Released;
    }
    if (craft.turboButton())
    {
        return argon::RaceSfx::Turbo::Pushing;
    }
    return craft.turboHeld() ? argon::RaceSfx::Turbo::Failing : argon::RaceSfx::Turbo::Released;
}

void RaceSounds::stepTurbo(const Race &race, const RaceState &state)
{
    if (state.held)
    {
        return;
    }
    for (int player = 0; player < state.humanPlayers && player < race.racerCount(); ++player)
    {
        sfx_.setTurbo(race.slotOf(player), padTurboOf(race.racer(player)));
    }
}

// weapon_launch 0x64e20: a projectile weapon's launch plays SF_SHIP_PROJECTILEFIRE, a beam's SF_SHIP_LASERFIRE, a
// shield's SF_GENERIC_RADIALLAUNCH (craft 0's only, unless split screen), at the shooter; each queues its travel loop
// with it (0x650aa / 0x651ab / 0x65399)
void RaceSounds::playLaunch(const argon::sound::TravelLaunch &launch, const RaceState &state)
{
    const int shooter = launch.key.craft;
    if (launch.weaponClass == kProjectileClass)
    {
        sfx_.playShipSound(argon::ShipSound::ProjectileFire, shooter);
        queueTravelLoop(launch, state);
    }
    else if (launch.weaponClass == kBeamClass)
    {
        sfx_.playShipSound(argon::ShipSound::LaserFire, shooter);
        queueTravelLoop(launch, state);
    }
    else if (launch.weaponClass == kShieldClass && argon::sound::shieldLaunchSounds(shooter, state.humanPlayers > 1))
    {
        sfx_.playAtCraft(argon::GenericSound::RadialLaunch, shooter);   // SF_GENERIC_RADIALLAUNCH (Rlaunch, 5.1 files)
        queueTravelLoop(launch, state);
    }
}

// 0xc3470: the player's pickup, at the player's craft
void RaceSounds::playPickup(argon::GenericSound id)
{
    sfx_.playAtCraft(id, 0);
}

// ---- the weapons' travel loops (SF_SHIP_WEAPONTRAVEL*, LOOPS 1): the voices of logic\travel_loops.hpp. Not in
// replays, whose projectile launches play no sounds here.

// 0xc9d10 (sound manager +0x133c): with every entry queued the earliest starts at once
void RaceSounds::queueTravelLoop(const argon::sound::TravelLaunch &launch, const RaceState &state)
{
    if (state.replaying)
    {
        return;
    }
    if (const auto early = travelLoops_.queue(launch, state.clock))
    {
        startTravelLoop(*early, state.clock);
    }
}

// sound_frame_update 0xc4f25 -> 0xc9cb0
void RaceSounds::startDueTravelLoops(float clock)
{
    for (const argon::sound::TravelLaunch &launch : travelLoops_.takeDue(clock))
    {
        startTravelLoop(launch, clock);
    }
}

// 0xc9dc0: the shooter's ship sound at the shooter (a beam's stays with it); a full set of voices loses its earliest
void RaceSounds::startTravelLoop(const argon::sound::TravelLaunch &launch, float clock)
{
    const int voice = sfx_.playShipSound(argon::sound::travelSound(launch.weaponClass), launch.key.craft);
    const int replaced = travelLoops_.holdVoice(launch.key, voice, clock);
    stopTravelVoices({replaced});
}

// 0xc9f80 from the projectile update (0x671a7) or the shield's sphere (0x69cd2)
void RaceSounds::moveTravelLoop(const argon::sound::TravelKey &key, const glm::vec3 &at)
{
    for (int voice : travelLoops_.voicesOf(key))
    {
        sfx_.moveTo(voice, at);
    }
}

// 0xc9f80 on a radial loop: its MINDIST / MAXDIST from the sphere's radius
void RaceSounds::spreadRadialTravelLoop(const argon::sound::TravelKey &key, float sphereRadius)
{
    const argon::SoundOptions &sound = sfx_.shipOptions(argon::ShipSound::TravelRadial, key.craft);
    const argon::sound::TravelDistances distances = argon::sound::radialTravelDistances(sphereRadius, sound.minDist, sound.maxDist);
    for (int voice : travelLoops_.voicesOf(key))
    {
        sfx_.setDistances(voice, distances.minDist, distances.maxDist);
    }
}

// 0xc9f00, from a projectile, beam or shield ending (0x65e60 -> 0x65f46)
void RaceSounds::stopTravelLoop(const argon::sound::TravelKey &key)
{
    stopTravelVoices(travelLoops_.stop(key));
}

// 0xc9c30 (from 0x3e9c4 by the restart, LOSEMENU 0x84c39, POSTABLE ON 0x8eab2, 0x421b2, 0x879fc): the race starting
// again, its end pages
void RaceSounds::stopTravelLoops()
{
    stopTravelVoices(travelLoops_.stopAll());
}

void RaceSounds::stopTravelVoices(const std::vector<int> &voices)
{
    for (int voice : voices)
    {
        sfx_.stopHandle(voice);
    }
}
