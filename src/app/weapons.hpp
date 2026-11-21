// Argon: the race's weapons as the game's craftweapon.cpp runs them (docs\weapons\README.md): data\weapon.nwp's records,
// each craft's weapon object, the volleys, projectiles and beams in flight, the hits, shields and knock-outs, the
// explosions, the lock-on and the AI's trigger, and a replay's weapons placed from its entries. The rules are logic\'s
// (weapon_book, shot_flight, craft_weapon, explosions, lock_on, ai_trigger, weapon_damage, weapon_rules); this keeps the
// order of a step and its side effects. What only the race around it knows, and what it changes there, goes through
// WeaponHost (App's: app\app_weapon_host.hpp).
//   firing       weapons.cpp          weapon_select 0x6c110 -> 0x6a390, the volley 0x69e30, the locks 0x6c420
//   in flight    weapon_shots.cpp     projectile 0x66d30, trail 0x66650, beam 0x68620, impact 0x65f80
//   hits         weapon_hits.cpp      0x6acf0, shields 0x6a390 / 0x69970, blasts 0x64ae0 / 0x7e410, knock-out 0x6a8a0
//   explosions   weapon_explosions.cpp 0x65f80 -> 0x7e0f0, going off 0x7de80
//   the AI       weapon_ai.cpp        the drones 0x58e40 / 0x3f3d0, the trigger 0x6e9e0
//   replays      weapon_replays.cpp   craft_weapon 0x641f0
#pragma once
#include "logic/player_pads.hpp"
#include "app/race_sounds.hpp"
#include "audio/sound_ids.hpp"
#include "effects/effect_slots.hpp"
#include "effects/weapon_sprites.hpp"
#include "launch/test_script.hpp"
#include "logic/ai_trigger.hpp"
#include "logic/attack_warnings.hpp"
#include "logic/craft_weapon.hpp"
#include "logic/engine_power_sound.hpp"
#include "logic/explosions.hpp"
#include "logic/lock_on.hpp"
#include "logic/particles.hpp"
#include "logic/race_crafts.hpp"
#include "logic/replay.hpp"
#include "logic/replay_weapons.hpp"
#include "logic/shot_flight.hpp"
#include "logic/split_screen.hpp"
#include "logic/travel_loops.hpp"
#include "logic/viewer_rand.hpp"
#include "logic/weapon_book.hpp"
#include "logic/weapon_rules.hpp"

#include <glm/glm.hpp>

#include <array>
#include <optional>
#include <string>
#include <vector>

class Race;
namespace argon
{
struct SimWorld;
}

// what the weapons ask of the race around them, and what they change there
class WeaponHost
{
  public:
    virtual ~WeaponHost() = default;

    // the clocks and the world
    virtual float clock() const = 0;       // the simulated seconds the weapons age by
    virtual float raceClock() const = 0;   // from the race's start (the logs)
    virtual RaceSounds::RaceState soundState() const = 0;
    virtual const argon::SimWorld &world() const = 0;
    virtual const argon::weapons::RaceCrafts &crafts() const = 0;

    // the crafts
    virtual glm::mat4 playerPose() const = 0;   // the player's craft as drawn this step
    virtual const argon::GameCraft &playerCraft() const = 0;
    virtual int craftBone(int slot, const std::string &name) const = 0;   // a bone of the slot's ship model, -1 none
    virtual std::optional<glm::mat4> craftBoneRest(int slot, int bone) const = 0;   // its model-space rest matrix
    virtual int pilotOf(int slot) const = 0;
    virtual argon::weapons::PowerLevels playerLoadout() const = 0;
    virtual void setPlayerLoadout(const argon::weapons::PowerLevels &levels) = 0;
    virtual int playerShieldLevel() const = 0;   // the Shield upgrade

    // the race's rules
    virtual bool modeHasWeapons() const = 0;
    virtual bool stillRacing(const argon::GameCraft &craft) const = 0;   // not past the race's laps
    virtual bool doublesDroneBlastBraking() const = 0;
    virtual bool nemesisRace() const = 0;
    virtual int speedClass() const = 0;
    virtual int trackId() const = 0;
    virtual bool playerDrives() const = 0;   // the humans fly their crafts (else the AI flies them all)
    virtual bool infiniteShield() const = 0;
    virtual bool raceRunning() const = 0;    // no race-end page up yet (0x52af9f)
    virtual bool replayPlaying() const = 0;
    virtual int humanPlayers() const = 0;
    virtual int cashValue(const std::string &name) const = 0;   // config\cashmodifiers.txt

    // what the weapons change around them
    virtual void recordWeaponEvent(const argon::replay::WeaponEvent &event) = 0;
    virtual void recordShieldValues() = 0;
    virtual void awardPoints(int points, int kind, int pilot) = 0;
    virtual void playerKnockedOutBy(int shooter) = 0;
    virtual void retireCraft(int slot) = 0;   // into the track's NET_CAGE
    virtual void loadExplosionModel(int model) = 0;
    virtual void emitTrailSprite(const argon::weapons::Shot &shot, const glm::vec3 &at) = 0;
    virtual void flashBeam(const argon::weapons::Shot &shot, argon::weapons::BeamStop stop) = 0;
    virtual void playShipSound(argon::ShipSound sound, int slot) = 0;
};

class Weapons
{
  public:
    using WeaponRec = argon::weapons::WeaponRec;
    using Shot = argon::weapons::Shot;
    using Volley = argon::weapons::Volley;
    using ShotLaunch = argon::weapons::ShotLaunch;
    using Segment = argon::weapons::Segment;
    using WorldHit = argon::weapons::WorldHit;
    using Explosion = argon::weapons::Explosion;
    using ActiveShield = argon::weapons::ActiveShield;
    using PlayerLocks = argon::weapons::PlayerLocks;
    using Loadout = argon::weapons::PowerLevels;

    static constexpr int kShips = argon::kEffectSlots;   // crafts with their own weapon object (index = grid slot)
    static constexpr int kRetaliateColour = 4;   // the retaliator's volley (fired by a shield, no colour of its own)
    static constexpr int kSpecialSlot = argon::weapons::kSpecialSlot;   // weapon.nwp record 9: the special

    // the race's own pieces the weapons drive
    struct Services
    {
        Race &race;
        RaceSounds &sounds;
        argon::Particles &particles;
        argon::WeaponSprites &sprites;
        argon::ViewerRand &rand;
        const TestScript &tests;
        const PlayerPads &pads;   // a multiplayer race: every craft's shield from the table
    };

    Weapons(WeaponHost &host, const Services &services);

    // ---- the race (weapons.cpp)
    void loadBook(const std::vector<uint8_t> &weaponNwp);
    const argon::weapons::WeaponBook &book() const;
    void resetForRace();
    void clearInFlight();   // the shots, volleys and explosions (a replay starts)
    void step();            // one 60 Hz step: the AI's trigger, the volleys, the shots, shields, explosions, effects
    void updateLocks();     // each player's crosshairs

    // ---- what the rest of the race reads
    argon::weapons::CraftWeapon &craft(int slot);
    const argon::weapons::CraftWeapon &craft(int slot) const;
    bool anyKnockedOut() const;
    const std::vector<Shot> &shots() const;
    const std::vector<Volley> &volleys() const;
    const argon::weapons::Explosions &explosions() const;
    const PlayerLocks &locks(int player) const;
    argon::AttackWarnings &warnings();
    const argon::AttackWarnings &warnings() const;

    float shieldPercent(int slot) const;   // huddata +0x44: the shield bar, the position list's energy bars
    float shieldSize(int pilot, int level) const;   // 0x64030
    bool shieldActive(int slot) const;
    argon::weapons::ShieldGuard shieldGuard(int slot) const;
    Loadout loadoutOf(int shooter) const;   // the AI's in its weapon object (0x6ba30), the player's the host's
    void setLoadout(int shooter, const Loadout &levels);
    const WeaponRec *pilotWeapon(int shooter, int slot) const;
    glm::mat4 shooterMatrix(int shooter) const;
    argon::weapons::DroneKind droneKindOf(int slot) const;   // 0x58e40
    argon::weapons::AiWeaponSetup aiWeaponSetupOf(int slot) const;   // 0x3f3d0: pickup caps and shield level

    // ---- what the race does to them
    bool fireWeapon(int shooter, int colour);   // 0x6d000
    void knockOut(int target, int shooter);     // 0x6a8a0

    // ---- a replay's weapons (weapon_replays.cpp): placed from its entries, the damage in its shield entries
    void setReplayedShield(const argon::replay::WeaponEvent &event);
    void fireReplayedWeapon(const argon::replay::WeaponEvent &event);
    void launchReplayedShot(const argon::replay::WeaponEvent &event);
    void moveReplayedShot(const argon::replay::WeaponEvent &event);
    void endReplayedShot(const argon::replay::WeaponEvent &event);
    void replayImpact(const argon::replay::WeaponEvent &event);
    void stepReplayed();   // the placed shots' trails, the beams on their bones, the explosions

    // ---- keys and pieces others share
    static argon::replay::ProjectileId projectileIdOf(const Shot &shot);
    static argon::sound::TravelKey travelKeyOf(const Shot &shot);
    static argon::sound::TravelLaunch travelLaunchOf(const WeaponRec &weaponType, int shooter, int projectile);
    int weaponBone(int slot, const WeaponRec &weaponType, int k) const;   // 0x64d20: projectile k is the ship's bone "<name><k>"
    glm::mat4 boneOnShip(int slot, int bone) const;   // where it sits on the ship as drawn; no bone: the ship's own matrix
    static argon::Particle particleOfSpark(const argon::weapons::SparkParticle &spark, float now);
    glm::vec3 trailVelocity(const Shot &shot);   // 0x66650: back along the forward, jittered

  private:
    // what a shot's beam reached
    struct BeamCast
    {
        argon::weapons::BeamStop stop = argon::weapons::BeamStop::Open;
        int craft = -1;   // the craft hit, -1 none
    };
    // one weapon's effects on one craft, from `at`
    struct WeaponHit
    {
        int target, shooter;
        const WeaponRec *weaponType;
        glm::vec3 at;
    };

    // firing (weapons.cpp)
    void resetShields();
    bool fireColour(int shooter, int colour);
    void fireSpecialsShield(int shooter);
    int armedWeaponSlot(int shooter, int colour) const;
    bool shotInFlight(int shooter, int colour) const;
    void fireVolley(const ShotLaunch &launch, std::vector<int> targets);
    void updateVolleys();
    void launchFromVolley(const Volley &volley, int projectile);
    Shot makeShot(const argon::GameCraft &shooterCraft, const ShotLaunch &launch, const glm::mat4 &start, int k) const;
    std::vector<int> lockedTargets(const ShotLaunch &launch) const;
    std::vector<int> lockCandidates(int slot) const;
    void updateLocksOf(int player);

    // in flight (weapon_shots.cpp)
    bool stepShot(Shot &shot);
    void steerShot(Shot &shot, float age);
    void moveShot(Shot &s);
    bool stepBeam(Shot &shot);
    BeamCast castBeam(Shot &shot);
    std::optional<glm::vec3> lockedTargetPosition(const Shot &s) const;
    int craftHitAlong(int shooter, const Segment &path, float radius) const;
    void warnCraftsNear(const glm::vec3 &point, int shooter);
    void warnCraftsOnPath(int shooter, const Segment &path, float radius);
    void emitShotTrail(Shot &shot);
    argon::Particle trailParticle(const Shot &shot, const glm::vec3 &at);
    bool impactCraft(const Shot &shot, int target);
    void bounceOff(Shot &shot, const WorldHit &wall);
    void finishShot(const Shot &shot, int kind);
    void logImpact(const Shot &shot, int kind) const;
    void recordShotState(const Shot &shot);

    // hits, shields, blasts (weapon_hits.cpp)
    void applyHit(const WeaponHit &hit);   // not while a replay plays: its shields come from its entries
    std::vector<const argon::original::NwpEffects *> shieldBonuses(int slot) const;
    void takeShieldDamage(int target, float percent);
    void leechTo(int shooter, float amount);
    void scoreHit(const WeaponHit &hit, float shieldPercentLost);
    void scoreKnockOut(const WeaponHit &hit);
    bool isPlayersNemesis(int slot) const;
    void raiseShield(int shooter, const WeaponRec &weaponType);
    void updateShieldSpheres();
    void reachWithSphere(int owner, float radius);
    glm::vec3 shieldCentre(int owner) const;
    void holdShieldTravelLoop(int owner);
    void retaliate(int victim, int attacker);
    void blastEveryCraft(const Explosion &e);
    void pushCraftsFromBlast(const Explosion &e);
    void afflict(int target, const argon::weapons::CraftEffects &hit);
    void flashShieldHit(int target);
    void logHit(const WeaponHit &hit, float lostPercent) const;
    void updateCraftEffects();
    void restartEngineAfter(int slot, float dt);
    void playEnginePowerUp(int slot);
    void playEnginePowerDown(int slot);
    void logEnginePower(const char *change, int slot) const;   // --autofire

    // explosions (weapon_explosions.cpp)
    void startImpactExplosions(const glm::vec3 &at, const WeaponRec &weaponType, int shooter, int kind);
    void startKnockOutExplosions(const glm::vec3 &at);
    void loadStartedExplosionModels();
    void addImpactFlashes(const glm::vec3 &at, const WeaponRec &weaponType);
    void updateExplosions();
    void goOff(const Explosion &explosion);
    void throwCraftHitSparks(const glm::vec3 &at);
    void throwImpactSparks(const glm::vec3 &at);

    // the AI (weapon_ai.cpp)
    float nemesisShareOf(int slot) const;
    void updateAiWeapons();
    void stepAiTrigger(int slot);
    argon::weapons::AiWants aiWants(int slot);
    argon::weapons::AiArmament aiArmament(int slot) const;
    argon::weapons::ArmedColour armedColour(int slot, int colour) const;
    bool lockedOnTo(int slot) const;   // in a player's lock lists or a shot's target

    // replays (weapon_replays.cpp)
    Shot *replayedShot(const argon::replay::ProjectileId &id);

    WeaponHost &host_;
    Race &race_;
    RaceSounds &sounds_;
    argon::Particles &particles_;
    argon::WeaponSprites &sprites_;
    argon::ViewerRand &rand_;
    const TestScript &tests_;
    const PlayerPads &pads_;

    argon::weapons::WeaponBook weaponBook;   // data\weapon.nwp: the records, resistances, explosion types
    std::vector<Shot> shots_;
    std::vector<Volley> volleys_;
    int volleyIds = 0;
    std::array<argon::weapons::CraftWeapon, kShips> craftWeapons{};   // their shield, knock-out, effects, flashes, AI levels
    argon::weapons::Explosions explosions_{weaponBook};
    std::array<PlayerLocks, argon::split::kMaxPlayers> playerLocks{};   // lock-on (0x6c420) and the HUD crosshairs
    argon::AttackWarnings attackWarnings;   // ATTACK WARNING: the crafts a shot puts in danger this step
    std::array<argon::EnginePowerSound, kShips> enginePowerSounds{};   // kept from boot, as the game's +0x201 + craft
};
