// Argon: App as the weapons' host (app\weapons.hpp WeaponHost): the race's clock, crafts, rules and what the weapons
// change around them (the points, the replay entries, the knock-out's cage, the explosion models, the sprites).
#pragma once
#include "app/app_race_crafts.hpp"
#include "app/weapons.hpp"
#include "logic/upgrade_rules.hpp"

class GameLogic;
class MultiplayerSession;
class PlayerKnockOut;
class PropModels;
class Race;
class ReplayRace;
class StartGrid;
struct RaceScoring;

namespace argon
{
class Audio;
struct GameProgress;
} // namespace argon

// what the weapons' host reads and changes
struct WeaponHostSources
{
    const float &clock;               // the effects' clock (App::simClock)
    const argon::SimWorld &world;
    const argon::GameCraft &player;   // racer 0 (App::sim)
    const glm::mat4 &playerPose;      // its pose at the last 60 Hz step (App::simCur)
    const Race &race;
    const Weapons &weapons;           // the crafts' frames and knock-outs
    PropModels &props;                // the craft models' bones, the explosion models
    StartGrid &startGrid;             // the slots' pilots
    int (&powerLevels)[3];            // the player's red / blue / yellow levels (App::hudWeapon)
    int &weaponSpecial;
    bool &hudSpecial;
    const int &hudLaps, &speedClass, &trackId;
    const bool &playerDrive;
    const argon::GameProgress &progress;   // the cheats
    const ReplayRace &replayRace;
    const MultiplayerSession &multiplayer;
    const RaceScoring &scoring;       // the cash values
    PlayerKnockOut &playerKnockOut;
    argon::Audio &audio;
};

// what the weapons' host asks App for
class RaceWeaponsHost
{
  public:
    virtual float raceClock() const = 0;
    virtual RaceSounds::RaceState raceSoundState() const = 0;
    virtual GameLogic &logic() const = 0;   // the game mode
    virtual int upgradeLevel(argon::upgrades::Upgrade upgrade) const = 0;
    virtual bool nemesisRace() const = 0;
    virtual bool raceRunning() const = 0;
    virtual void recordWeaponEvent(const argon::replay::WeaponEvent &event) = 0;
    virtual void recordShieldValues() = 0;
    virtual void awardPoints(int points, int kind, int pilot) = 0;
    virtual void retireCraft(int slot) = 0;
    virtual void emitTrailSprite(const argon::weapons::Shot &shot, const glm::vec3 &at) = 0;
    virtual void flashBeam(const argon::weapons::Shot &shot, argon::weapons::BeamStop stop) = 0;

  protected:
    ~RaceWeaponsHost() = default;
};

class AppWeaponHost final : public WeaponHost
{
  public:
    AppWeaponHost(RaceWeaponsHost &host, const WeaponHostSources &sources);

    float clock() const override;
    float raceClock() const override;
    RaceSounds::RaceState soundState() const override;
    const argon::SimWorld &world() const override;
    const argon::weapons::RaceCrafts &crafts() const override;

    glm::mat4 playerPose() const override;
    const argon::GameCraft &playerCraft() const override;
    int craftBone(int slot, const std::string &name) const override;
    std::optional<glm::mat4> craftBoneRest(int slot, int bone) const override;
    int pilotOf(int slot) const override;
    argon::weapons::PowerLevels playerLoadout() const override;
    void setPlayerLoadout(const argon::weapons::PowerLevels &levels) override;
    int playerShieldLevel() const override;

    bool modeHasWeapons() const override;
    bool stillRacing(const argon::GameCraft &craft) const override;
    bool doublesDroneBlastBraking() const override;
    bool nemesisRace() const override;
    int speedClass() const override;
    int trackId() const override;
    bool playerDrives() const override;
    bool infiniteShield() const override;
    bool raceRunning() const override;
    bool replayPlaying() const override;
    int humanPlayers() const override;
    int cashValue(const std::string &name) const override;

    void recordWeaponEvent(const argon::replay::WeaponEvent &event) override;
    void recordShieldValues() override;
    void awardPoints(int points, int kind, int pilot) override;
    void playerKnockedOutBy(int shooter) override;
    void retireCraft(int slot) override;
    void loadExplosionModel(int model) override;
    void emitTrailSprite(const argon::weapons::Shot &shot, const glm::vec3 &at) override;
    void flashBeam(const argon::weapons::Shot &shot, argon::weapons::BeamStop stop) override;
    void playShipSound(argon::ShipSound sound, int slot) override;

  private:
    RaceWeaponsHost &host_;
    WeaponHostSources sources_;
    AppRaceCrafts crafts_;
};
