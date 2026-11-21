// Argon: App as the weapons' host (app_weapon_host.hpp).
#include "app/app_weapon_host.hpp"

#include "audio/audio.hpp"
#include "audio/race_sfx.hpp"
#include "logic/game_logic.hpp"
#include "logic/game_progress.hpp"
#include "logic/multiplayer_session.hpp"
#include "logic/race_clock.hpp"
#include "logic/race_scoring.hpp"
#include "logic/replay_race.hpp"
#include "logic/start_grid.hpp"
#include "render/prop_models.hpp"

AppWeaponHost::AppWeaponHost(RaceWeaponsHost &host, const WeaponHostSources &sources) : host_(host), sources_(sources), crafts_(sources.race, sources.weapons)
{
}

float AppWeaponHost::clock() const
{
    return sources_.clock;
}

float AppWeaponHost::raceClock() const
{
    return host_.raceClock();
}

RaceSounds::RaceState AppWeaponHost::soundState() const
{
    return host_.raceSoundState();
}

const argon::SimWorld &AppWeaponHost::world() const
{
    return sources_.world;
}

const argon::weapons::RaceCrafts &AppWeaponHost::crafts() const
{
    return crafts_;
}

glm::mat4 AppWeaponHost::playerPose() const
{
    return sources_.playerPose;
}

const argon::GameCraft &AppWeaponHost::playerCraft() const
{
    return sources_.player;
}

int AppWeaponHost::craftBone(int slot, const std::string &name) const
{
    const Found<Track> model = sources_.props.loadedCraft(sources_.startGrid.pilotOf(slot));
    return model ? model->findBone(name) : -1;
}

std::optional<glm::mat4> AppWeaponHost::craftBoneRest(int slot, int bone) const
{
    const Found<Track> model = sources_.props.loadedCraft(sources_.startGrid.pilotOf(slot));
    if (!model || bone < 0 || bone >= int(model->boneRest.size()))
    {
        return std::nullopt;
    }
    return model->boneRest[size_t(bone)];
}

int AppWeaponHost::pilotOf(int slot) const
{
    return argon::pilotIndex(sources_.startGrid.pilotOf(slot));
}

argon::weapons::PowerLevels AppWeaponHost::playerLoadout() const
{
    return argon::weapons::PowerLevels{sources_.powerLevels[0], sources_.powerLevels[1], sources_.powerLevels[2], sources_.weaponSpecial};
}

void AppWeaponHost::setPlayerLoadout(const argon::weapons::PowerLevels &levels)
{
    sources_.powerLevels[0] = levels.red;
    sources_.powerLevels[1] = levels.blue;
    sources_.powerLevels[2] = levels.yellow;
    sources_.weaponSpecial = levels.special;
    sources_.hudSpecial = sources_.weaponSpecial > 0;
}

int AppWeaponHost::playerShieldLevel() const
{
    return host_.upgradeLevel(argon::upgrades::kShield);
}

bool AppWeaponHost::modeHasWeapons() const
{
    return host_.logic().hasWeapons();
}

bool AppWeaponHost::stillRacing(const argon::GameCraft &craft) const
{
    return !host_.logic().hasLapLimit() || craft.laps() < sources_.hudLaps;
}

bool AppWeaponHost::doublesDroneBlastBraking() const
{
    return host_.logic().doublesDroneBlastBraking();
}

bool AppWeaponHost::nemesisRace() const
{
    return host_.nemesisRace();
}

int AppWeaponHost::speedClass() const
{
    return sources_.speedClass;
}

int AppWeaponHost::trackId() const
{
    return sources_.trackId;
}

bool AppWeaponHost::playerDrives() const
{
    return sources_.playerDrive;
}

bool AppWeaponHost::infiniteShield() const
{
    return sources_.progress.cheatsOn[5];   // 0x1bbbf0
}

bool AppWeaponHost::raceRunning() const
{
    return host_.raceRunning();
}

bool AppWeaponHost::replayPlaying() const
{
    return sources_.replayRace.playing();
}

int AppWeaponHost::humanPlayers() const
{
    return sources_.multiplayer.players();
}

int AppWeaponHost::cashValue(const std::string &name) const
{
    return sources_.scoring.cash.value(name);
}

void AppWeaponHost::recordWeaponEvent(const argon::replay::WeaponEvent &event)
{
    host_.recordWeaponEvent(event);
}

void AppWeaponHost::recordShieldValues()
{
    host_.recordShieldValues();
}

void AppWeaponHost::awardPoints(int points, int kind, int pilot)
{
    host_.awardPoints(points, kind, pilot);
}

void AppWeaponHost::playerKnockedOutBy(int shooter)
{
    sources_.playerKnockOut.knockedOutBy(shooter, sources_.clock);
}

void AppWeaponHost::retireCraft(int slot)
{
    host_.retireCraft(slot);
}

void AppWeaponHost::loadExplosionModel(int model)
{
    sources_.props.explosion(model);
}

void AppWeaponHost::emitTrailSprite(const argon::weapons::Shot &shot, const glm::vec3 &at)
{
    host_.emitTrailSprite(shot, at);
}

void AppWeaponHost::flashBeam(const argon::weapons::Shot &shot, argon::weapons::BeamStop stop)
{
    host_.flashBeam(shot, stop);
}

void AppWeaponHost::playShipSound(argon::ShipSound sound, int slot)
{
    sources_.audio.raceSfx().playShipSound(sound, slot);
}
