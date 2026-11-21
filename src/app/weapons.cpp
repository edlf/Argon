// Argon: the weapon records, loadouts, firing and lock-on (craftweapon.cpp in the game, docs\weapons\README.md).
//   records  data\weapon.nwp (loader 0x640c0): 16 pilot blocks of 11 records (0x334 bytes)
//   fire     weapon_select 0x6c110 -> 0x6a390 (the volley: projectile k leaves k x +0x170 later, 0x69e30)
//   locks    0x6c420 (targets), HUD crosshairs 0x4e7f0
// Projectiles and beams: weapon_shots.cpp; hits, shields and knock-outs: weapon_hits.cpp; the AI: weapon_ai.cpp.
#include "app/weapons.hpp"

#include "app/app_race_crafts.hpp"
#include "logic/race.hpp"
#include "original/record_read.hpp"
#include "original/weapon_records.hpp"

namespace
{
const int kWeaponSlots = argon::weapons::WeaponBook::kSlots;
}  // namespace

Weapons::Weapons(WeaponHost &host, const Services &services)
    : host_(host), race_(services.race), sounds_(services.sounds), particles_(services.particles), sprites_(services.sprites), rand_(services.rand),
      tests_(services.tests), pads_(services.pads)
{
}

void Weapons::loadBook(const std::vector<uint8_t> &weaponNwp)
{
    weaponBook.read(weaponNwp);
}

const argon::weapons::WeaponBook &Weapons::book() const
{
    return weaponBook;
}

argon::weapons::CraftWeapon &Weapons::craft(int slot)
{
    return craftWeapons[size_t(std::clamp(slot, 0, kShips - 1))];
}

const argon::weapons::CraftWeapon &Weapons::craft(int slot) const
{
    return craftWeapons[size_t(std::clamp(slot, 0, kShips - 1))];
}

bool Weapons::anyKnockedOut() const
{
    for (const argon::weapons::CraftWeapon &weapon : craftWeapons)
    {
        if (weapon.knockedOut())
        {
            return true;
        }
    }
    return false;
}

const std::vector<Weapons::Shot> &Weapons::shots() const
{
    return shots_;
}

const std::vector<Weapons::Volley> &Weapons::volleys() const
{
    return volleys_;
}

const argon::weapons::Explosions &Weapons::explosions() const
{
    return explosions_;
}

const Weapons::PlayerLocks &Weapons::locks(int player) const
{
    return playerLocks[size_t(std::clamp(player, 0, argon::split::kMaxPlayers - 1))];
}

argon::AttackWarnings &Weapons::warnings()
{
    return attackWarnings;
}

const argon::AttackWarnings &Weapons::warnings() const
{
    return attackWarnings;
}

argon::replay::ProjectileId Weapons::projectileIdOf(const Shot &shot)
{
    return {shot.shooter, shot.weaponType->slot, shot.index};
}

argon::sound::TravelKey Weapons::travelKeyOf(const Shot &shot)
{
    return {shot.shooter, shot.weaponType->slot, shot.index};
}

argon::sound::TravelLaunch Weapons::travelLaunchOf(const WeaponRec &weaponType, int shooter, int projectile)
{
    return argon::sound::TravelLaunch{{shooter, weaponType.slot, projectile}, weaponType.cls};
}

float Weapons::shieldSize(int pilot, int level) const
{
    return weaponBook.shieldSize(pilot, level);
}

float Weapons::shieldPercent(int slot) const
{
    return craftWeapons[size_t(std::clamp(slot, 0, kShips - 1))].shieldPercent();
}

// each craft starts with its full shield (race setup 0x3f3d0: the player's Shield upgrade level, the others' by speed
// class and kind; in split screen every craft's from the table, slot 0 too)
void Weapons::resetShields()
{
    for (int slot = 0; slot < kShips; ++slot)
    {
        const bool ownUpgrades = slot == 0 && !pads_.multiplayerRace();
        const int level = ownUpgrades ? host_.playerShieldLevel() : aiWeaponSetupOf(slot).shieldLevel;
        craftWeapons[slot].equipShield(shieldSize(host_.pilotOf(slot), level));
    }
}

void Weapons::resetForRace()
{
    clearInFlight();
    sprites_.clear();
    resetShields();
    for (argon::weapons::CraftWeapon &weapon : craftWeapons)
    {
        weapon.resetForRace();
    }
    playerLocks.fill(PlayerLocks{});
}

Weapons::Loadout Weapons::loadoutOf(int shooter) const
{
    if (shooter != 0)
    {
        return craftWeapons[std::clamp(shooter, 0, kShips - 1)].aiLoadout();
    }
    return host_.playerLoadout();
}

void Weapons::setLoadout(int shooter, const Loadout &levels)
{
    if (shooter != 0)
    {
        craftWeapons[std::clamp(shooter, 0, kShips - 1)].setAiLoadout(levels);
        return;
    }
    host_.setPlayerLoadout(levels);
}

// weapon_select 0x6c110: colour 0 red / 1 yellow / 2 blue / 3 special; slot = (level - 1) x 3 + colour, 9 special
int Weapons::armedWeaponSlot(int shooter, int colour) const
{
    const Loadout loadout = loadoutOf(shooter);
    if (colour == 3)
    {
        return loadout.special > 0 ? kSpecialSlot : -1;
    }
    const int level = colour == 0 ? loadout.red : colour == 1 ? loadout.yellow : colour == 2 ? loadout.blue : 0;
    return level > 0 ? (std::min(level, 3) - 1) * 3 + colour : -1;
}

const Weapons::WeaponRec *Weapons::pilotWeapon(int shooter, int slot) const
{
    const Found<const argon::GameCraft> craft = race_.craftInSlot(shooter);
    if (!craft || !weaponBook.hasRecords() || slot < 0 || slot >= kWeaponSlots)
    {
        return nullptr;
    }
    return &weaponBook.record(std::clamp(craft->pilot(), 0, 15), slot);
}

// that colour's weapon slot object is still active: a shield still up, a volley still launching or a projectile out
bool Weapons::shotInFlight(int shooter, int colour) const
{
    if (shieldActive(shooter) && craftWeapons[shooter].shield().weaponType->slot % 3 == colour)
    {
        return true;
    }
    for (auto &volley : volleys_)
    {
        if (volley.launch.shooter == shooter && volley.launch.colour == colour)
        {
            return true;
        }
    }
    for (auto &shot : shots_)
    {
        if (shot.shooter == shooter && shot.colour == colour)
        {
            return true;
        }
    }
    return false;
}

glm::mat4 Weapons::shooterMatrix(int shooter) const
{
    glm::mat4 pose = shooter == 0 ? host_.playerPose() : glm::mat4(1.f);
    for (auto &drone : race_.grid())
    {
        if (drone.slot == shooter)
        {
            pose = drone.cur;
        }
    }
    return pose;
}

// 0x6d000: a held special turns any request into the special, then the yellow colour fires at level 3 (0x6d0b1);
// nothing while the craft is out (+0x1145) or the race has no weapons
bool Weapons::fireWeapon(int shooter, int colour)
{
    if (!host_.modeHasWeapons() || craftWeapons[std::clamp(shooter, 0, kShips - 1)].knockedOut())
    {
        return false;
    }
    const bool holdsSpecial = loadoutOf(shooter).special > 0;
    const bool fired = fireColour(shooter, argon::weapons::fireColour(colour, holdsSpecial));
    if (holdsSpecial)
    {
        fireSpecialsShield(shooter);
    }
    return fired;
}

void Weapons::fireSpecialsShield(int shooter)
{
    setLoadout(shooter, argon::weapons::levelsForSpecialsShield(loadoutOf(shooter)));
    fireColour(shooter, argon::weapons::kYellowColour);
}

// weapon_select 0x6c110: blocked while this colour's last shot is still out; the levels empty after a shot (0x6c33e)
bool Weapons::fireColour(int shooter, int colour)
{
    const int slot = armedWeaponSlot(shooter, colour);
    const WeaponRec *weaponType = pilotWeapon(shooter, slot);
    if (!weaponType || weaponType->count <= 0 || shotInFlight(shooter, colour))
    {
        return false;
    }
    const ShotLaunch launch{shooter, colour, weaponType};
    host_.recordWeaponEvent(argon::replay::fireEvent(shooter, weaponType->slot));   // 0x6a390
    if (weaponType->cls == 2)
    {
        raiseShield(shooter, *weaponType);
    }
    else
    {
        fireVolley(launch, lockedTargets(launch));
    }
    if (tests_.autoFire)
    {
        std::fprintf(stderr, "fire: shooter %d colour %d slot %d %s at %.2f s\n", shooter, colour, slot, weaponType->name.c_str(), host_.clock());
    }
    setLoadout(shooter, argon::weapons::levelsAfterFiring(loadoutOf(shooter), slot));
    return true;
}

void Weapons::fireVolley(const ShotLaunch &launch, std::vector<int> targets)
{
    Volley volley;
    volley.launch = launch;
    volley.id = ++volleyIds;
    volley.firedAt = host_.clock();
    volley.targets = std::move(targets);
    volley.targets.resize(size_t(launch.weaponType->count), -1);
    volleys_.push_back(std::move(volley));
}

// 0x69e30, each frame: projectile k leaves once k x stagger has passed; a repeating weapon relaunches it (the old one
// done first, 0x65e60) every count x stagger while the repeat time lasts
void Weapons::updateVolleys()
{
    for (size_t i = 0; i < volleys_.size();)
    {
        while (argon::weapons::launchDue(volleys_[i], host_.clock()))
        {
            launchFromVolley(volleys_[i], volleys_[i].launched % volleys_[i].launch.weaponType->count);
            ++volleys_[i].launched;
        }
        if (argon::weapons::volleyGoesOn(volleys_[i], host_.clock()))
        {
            ++i;
            continue;
        }
        volleys_[i] = std::move(volleys_.back());
        volleys_.pop_back();
    }
}

void Weapons::launchFromVolley(const Volley &volley, int projectile)
{
    const Found<const argon::GameCraft> craft = race_.craftInSlot(volley.launch.shooter);
    if (!craft || craftWeapons[volley.launch.shooter].knockedOut())
    {
        return;
    }
    for (size_t i = 0; i < shots_.size(); ++i)
    {
        if (shots_[i].volley == volley.id && shots_[i].index == projectile)
        {
            sounds_.stopTravelLoop(travelKeyOf(shots_[i]));
            shots_[i] = shots_.back();
            shots_.pop_back();
            break;
        }
    }
    const int bone = weaponBone(volley.launch.shooter, *volley.launch.weaponType, projectile);
    Shot shot = makeShot(*craft, volley.launch, boneOnShip(volley.launch.shooter, bone), projectile);
    shot.bone = bone;
    shot.volley = volley.id;
    shot.target = volley.targets[size_t(projectile)];
    const Found<const argon::GameCraft> target = shot.target >= 0 ? race_.craftInSlot(shot.target) : std::nullopt;
    argon::weapons::aimAtLaunch(shot, shooterMatrix(volley.launch.shooter), target ? std::optional<glm::vec3>(target->position()) : std::nullopt);
    shots_.push_back(shot);
    host_.recordWeaponEvent(argon::replay::launchEvent(projectileIdOf(shot), shot.target));   // weapon_launch 0x659eb
    sounds_.playLaunch(travelLaunchOf(*volley.launch.weaponType, volley.launch.shooter, projectile), host_.soundState());
}

// 0x64d20: projectile k is the bone "<name><k>" of the ship model (a shield's is k = 0)
int Weapons::weaponBone(int slot, const WeaponRec &weaponType, int projectile) const
{
    return host_.craftBone(slot, weaponType.name + std::to_string(projectile));
}

// where a weapon bone (weaponBone) sits on the ship; no bone: the ship's own matrix
glm::mat4 Weapons::boneOnShip(int slot, int bone) const
{
    const glm::mat4 ship = shooterMatrix(slot);
    const std::optional<glm::mat4> rest = bone >= 0 ? host_.craftBoneRest(slot, bone) : std::nullopt;
    return rest ? ship * *rest : ship;
}

Weapons::Shot Weapons::makeShot(const argon::GameCraft &shooterCraft, const ShotLaunch &launch, const glm::mat4 &start, int projectile) const
{
    const argon::weapons::ShooterMotion motion{shooterCraft.forwardSpeed(), shooterCraft.velocity(), glm::vec3(shooterCraft.matrix()[2])};
    return argon::weapons::makeShot(launch, argon::weapons::ShotStart{start, projectile, host_.clock()}, motion);
}

// 0x6c110: lock-on weapons give projectile k the k-th locked target (nearest first), the rest the nearest one
std::vector<int> Weapons::lockedTargets(const ShotLaunch &launch) const
{
    const std::vector<int> locks = launch.weaponType->lockOn ? lockCandidates(launch.shooter) : std::vector<int>{};
    return argon::weapons::volleyTargets(locks, launch.weaponType->count);
}

void Weapons::step()
{
    sprites_.dropFlashes();   // 0xbd5f0: the last frame's one-frame quads
    updateAiWeapons();
    updateVolleys();
    for (size_t i = 0; i < shots_.size();)
    {
        if (!stepShot(shots_[i]))
        {
            recordShotState(shots_[i]);
            ++i;
            continue;
        }
        host_.recordWeaponEvent(argon::replay::doneEvent(projectileIdOf(shots_[i])));
        sounds_.stopTravelLoop(travelKeyOf(shots_[i]));
        shots_[i] = shots_.back();
        shots_.pop_back();
    }
    updateShieldSpheres();
    updateExplosions();
    updateCraftEffects();
    host_.recordShieldValues();
}

// 0x6c420: other crafts still racing, within 500 m and ahead (direction . forward > 0.85), nearest first, at most 6
std::vector<int> Weapons::lockCandidates(int slot) const
{
    return argon::weapons::lockCandidates(slot, host_.crafts());
}

// 0x6c420 for each player's craft (one in single player): for each colour whose armed weapon locks (+0x150), up to
// +0x168 targets; none while that colour's last shot is out. Then the HUD (0x4e7f0, crosshairs)
void Weapons::updateLocks()
{
    for (int player = 0; player < argon::split::kMaxPlayers; ++player)
    {
        if (player < host_.humanPlayers() && player < race_.racerCount())
        {
            updateLocksOf(player);
        }
        else
        {
            playerLocks[size_t(player)] = PlayerLocks{};
        }
    }
}

void Weapons::updateLocksOf(int player)
{
    if (!weaponBook.hasRecords())
    {
        return;
    }
    const int shooter = race_.slotOf(player);
    const std::vector<int> cand = lockCandidates(shooter);
    const bool special = loadoutOf(shooter).special > 0;
    // list 0 red (+0x4c), 1 blue (+0x50), 2 yellow (+0x54); colour ids 0 / 2 / 1
    const int colourOf[PlayerLocks::kLists] = {0, 2, 1};
    PlayerLocks::Lists lists;
    for (int li = 0; li < PlayerLocks::kLists; ++li)
    {
        const int colour = colourOf[li];
        const int slot = special ? kSpecialSlot : armedWeaponSlot(shooter, colour);
        const WeaponRec *weaponType = pilotWeapon(shooter, slot);
        if (!weaponType || !weaponType->lockOn || shotInFlight(shooter, colour))
        {
            continue;
        }
        for (size_t i = 0; i < cand.size() && int(i) < weaponType->count; ++i)
        {
            lists[size_t(li)].push_back(cand[i]);
        }
    }
    playerLocks[size_t(player)].update(lists, host_.clock());
}

// the shots, volleys and explosions in flight and their travel loops
void Weapons::clearInFlight()
{
    shots_.clear();
    volleys_.clear();
    explosions_.clear();
    sounds_.stopTravelLoops();
}
