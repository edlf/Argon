// Argon: the race's event sounds (docs\audio\race-sounds.md "Race sounds"; the rules: logic\sound_rules.hpp, the voices:
// audio\race_sfx.hpp). Every craft's hits, scrapes, craft contacts and splashes (0xc2b50, craft_water 0x61340), the
// weapons' sounds (incoming 0xc2f90, player hit 0xc2fe0, projectile / mega-up impacts 0xc3050, the beam's hit
// 0x6908a), the weapons' travel loops (0xc9c30..0xc9f80), the turbo and the pickups' (0xc3470). Sounds follow the
// crafts by grid slot.
#pragma once
#include <array>
#include <vector>

#include <glm/glm.hpp>

#include "audio/race_sfx.hpp"
#include "audio/sound_ids.hpp"
#include "craft/craftsim.hpp"
#include "logic/sound_rules.hpp"
#include "logic/travel_loops.hpp"

class Race;

class RaceSounds
{
  public:
    // what the sounds ask of the race each call (App::raceSoundState)
    struct RaceState
    {
        int humanPlayers = 1;    // the split screen's players, grid slots 0..n-1
        bool racing = false;     // no race end page up (0x52afa8 = 0)
        bool replaying = false;  // the attract demo or a replay places the crafts (0x52afac)
        bool held = false;       // the grid intro or the countdown (0x52af9e)
        float clock = 0.f;       // the race's simulated seconds
    };

    explicit RaceSounds(argon::RaceSfx &sfx) : sfx_(sfx)
    {
    }
    void useImpactSettings(const argon::sound::ImpactSettings &settings)   // audio\f_settings.txt
    {
        impactSettings_ = settings;
    }

    void stepCrafts(const Race &race, const RaceState &state);   // each 60 Hz step after the crafts moved (0xc4bf0)
    void stepTurbo(const Race &race, const RaceState &state);
    void playIncoming(int target, const RaceState &state);
    void playPlayerHit(int target, const glm::vec3 &at, const RaceState &state);
    void playProjectileImpact(const glm::vec3 &at);
    void playMegaImpact(const glm::vec3 &at);   // the special's
    void playLaserImpact(int target, const RaceState &state);
    void playLaunch(const argon::sound::TravelLaunch &launch, const RaceState &state);
    void playPickup(argon::GenericSound id);

    void moveTravelLoop(const argon::sound::TravelKey &key, const glm::vec3 &at);
    void spreadRadialTravelLoop(const argon::sound::TravelKey &key, float sphereRadius);
    void stopTravelLoop(const argon::sound::TravelKey &key);
    void stopTravelLoops();


  private:
    void playContactSounds(int slot, const argon::GameCraft::SoundContacts &contacts);
    void playWorldContact(int slot, const argon::sound::CraftContact &contact);
    void playSplash(int slot, float verticalSpeed);
    void playHitSound(argon::GenericSound id, int slot, float volumeMb);
    argon::sound::VolumeRange hitVolumeRange(argon::GenericSound id) const;
    void holdIncomingWarning(int slot);
    void queueTravelLoop(const argon::sound::TravelLaunch &launch, const RaceState &state);
    void startDueTravelLoops(float clock);
    void startTravelLoop(const argon::sound::TravelLaunch &launch, float clock);
    void stopTravelVoices(const std::vector<int> &voices);

    argon::RaceSfx &sfx_;
    argon::sound::ImpactSettings impactSettings_;
    std::array<int, 16> incomingFrames_{};   // 0xc2f90's per-craft hold (sound manager +0x208)
    argon::sound::TravelLoops travelLoops_;
};
