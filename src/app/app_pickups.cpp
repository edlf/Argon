// Argon: what the power-up pickups give as the game runs them (see docs\effects\README.md, "Pickups"); which pickup goes
// and comes back is argon::Pickups (logic\pickups.hpp), the particles argon::Particles (logic\particles.hpp).
//   pickups_collide 0x8fcd0  (called from the craft weapon update 0x6d000 with the ship position, radius 14)
//   weapon_pickup   0x6ba30  (power-up levels, special)        pickup_burst 0x88750 (starburst particles)
//   particles       0xbc640 update / 0xbc900 draw (system 0x1baa28: data\hud\starburst.dds, additive points)
#include "app/app.hpp"

using argon::gfx::SamplerMode;

using argon::PickupChange;
using argon::PickupType;

// race setup 0x3f3d0 -> 0x3f650 (EAX = the player's Weapon Strength level): the pickup caps red / yellow / blue and
// the special cap per level 0..7 (the AI crafts' caps come from the speed class and their kind: aiWeaponSetupOf)
void App::applyUpgradeCaps()
{
    if (!powerUps.capsFromUpgrades)
    {
        powerUps.caps.red = powerUps.caps.blue = powerUps.caps.yellow = powerUps.editorLevelCap;
        return;
    }
    if (playerPads.multiplayerRace())
    {   // split screen: slot 0 keeps the table's caps as every craft (0x3f3d0 skips 0x3f650 in mode 4)
        const argon::weapons::AiWeaponSetup setup = weapons.aiWeaponSetupOf(0);
        powerUps.caps = {setup.levelCap, setup.levelCap, setup.levelCap, setup.specialCap};
        return;
    }
    powerUps.caps = argon::upgrades::pickupCaps(upgradeLevel(argon::upgrades::kWeaponStrength));
}

// data\weapon.nwp: the records, resistances and explosion types (loader 0x640c0)
void App::loadWeapons()
{
    weapons.loadBook(readBytes(game + "/data/weapon.nwp"));
}

void App::resetPickups()
{
    pickups.place(track.data.pickups);
    applyUpgradeCaps();
    particles.clear();
    scoring.points.resetHud();   // 0x49360: with the HUD
    weapons.resetForRace();
    scoring.airTimer = 0;
    scoring.lapsSeen = 0;
    powerUps.startRace(tests.startPowerups);   // --powerups (tests), else 0
}

// 0x6d000 / 0x8fcd0, once per 60 Hz step with the player's position
void App::updatePickups(const glm::vec3 &ship)
{
    stepParticles(ship);
    if (!logic().hasPickups())
    {
        return;
    }
    if (pickups.count() != track.data.pickups.size())
    {
        resetPickups();
    }
    giveTestSpecial();
    pickups.startLap(sim.laps());
    if (!pickupLogic)
    {
        return;
    }
    for (const PickupChange &change : pickups.stepPlayer(ship, simClock))
    {
        if (change.kind == PickupChange::Kind::Back)
        {
            burstOnPickup(change);
            recordWeaponEvent(argon::replay::pickupBackEvent(change.index));
            continue;
        }
        recordWeaponEvent(argon::replay::pickupTakenEvent(change.index, 0));
        givePlayerPickup(change, ship);
    }
}

// a power chip: points only (0x83ea0, "Points Bonus"); else the levels (0x6ba30), the burst on the ship, the sound
// (0xc3470) and the points (0x83ed0 mega-up / 0x83e70)
void App::givePlayerPickup(const PickupChange &taken, const glm::vec3 &ship)
{
    if (taken.type == PickupType::PowerChip)
    {
        raceSounds.playPickup(argon::GenericSound::ChipPickup);
        awardPoints(scoring.cash.value("powerchip_bonus"), 8);
        return;
    }
    const bool megaUp = taken.type == PickupType::MegaUp;
    raisePlayerPowerUps(int(taken.type));
    burstOnCraft(taken, ship, 0);
    raceSounds.playPickup(megaUp ? argon::GenericSound::MegaPickup : argon::GenericSound::PowerPickup);
    awardPoints(scoring.cash.value(megaUp ? "megaup_bonus" : "pickup_bonus"), megaUp ? 10 : 9);
}

// pickup_burst 0x88750 as the game calls it: collected, mode 3 (implode onto the moving ship), speed 10; respawned,
// mode 1 (implode onto the pickup), speed 20
void App::burstOnCraft(const PickupChange &taken, const glm::vec3 &craft, int slot)
{
    particles.burst(argon::ParticleBurst{craft, slot, 10.f, argon::pickupBurstColour(taken.type)}, simClock, viewerRand);
}

void App::burstOnPickup(const PickupChange &back)
{
    particles.burst(argon::ParticleBurst{back.pos, -1, 20.f, argon::pickupBurstColour(back.type)}, simClock, viewerRand);
}

// 0x6ba30: levels red +0xa4 / blue +0xa8 / yellow +0xac (viewer hudWeapon[0..2] = red / blue / yellow), capped per
// player (0x52cd0c / 12 / 18); a mega-up raises all three; with all three at 3 any pickup adds a special (+0xb0,
// capped by 0x52cd1e)
void App::raisePlayerPowerUps(int type)
{
    weapons.setLoadout(0, argon::weapons::raisePowerLevels(weapons.loadoutOf(0), type, powerUps.caps));
}

// --special-at (tests), once: the levels a mega-up leaves at 3 / 3 / 3 with Weapon Strength 7 (0x6ba30, special cap
// 1 from 0x3f650): all three at 3 and a special held
void App::giveTestSpecial()
{
    if (tests.specialAt < 0.f || raceClock() < tests.specialAt)
    {
        return;
    }
    tests.specialAt = -1.f;
    const int top = 3;
    weapons.setLoadout(0, Weapons::Loadout{top, top, top, 1});
}

// a replay's "pickup taken" entry (signal 8): gone, with the burst on the craft that took it. The player's power-up
// levels rise with it: in the game craft_weapon_update's own pickup test (0x6d0e6 -> weapon_pickup 0x6ba30, not
// held back by the replay flag 0x52afac) credits the replayed craft as it passes the pickup, and the replayed fires
// (0x6a390) leave the levels as they are, so View Replay's power-up panel fills and stays full
void App::takeReplayedPickup(int pickup, int slot)
{
    if (pickups.count() != track.data.pickups.size())
    {
        resetPickups();
    }
    const std::optional<PickupChange> taken = pickups.takeReplayed(pickup, simClock);
    if (!taken)
    {
        return;
    }
    if (const Found<const argon::GameCraft> taker = race.craftInSlot(slot))
    {
        burstOnCraft(*taken, taker->position(), slot);
    }
    if (slot == 0 && taken->type != PickupType::PowerChip)
    {
        raisePlayerPowerUps(int(taken->type));
    }
}

// signal 9: back on the track, with the respawn burst
void App::bringReplayedPickupBack(int pickup)
{
    if (const std::optional<PickupChange> back = pickups.bringBack(pickup))
    {
        burstOnPickup(*back);
    }
}

// an AI craft (0x6d000 with the AI flag): weapon_pickup (0x6ba30) raises its levels (capped by 0x3f3d0:
// aiWeaponSetupOf) and bursts on that ship. Sound only for the player.
void App::aiTakePickups(const glm::vec3 &ship, int slot)
{
    if (!pickupLogic || pickups.count() != track.data.pickups.size() || weapons.craft(slot).knockedOut())
    {
        return;
    }
    for (const PickupChange &taken : pickups.takeByAi(ship, simClock))
    {
        recordWeaponEvent(argon::replay::pickupTakenEvent(taken.index, slot));
        const argon::weapons::AiWeaponSetup caps = weapons.aiWeaponSetupOf(slot);
        const Weapons::Loadout capped{caps.levelCap, caps.levelCap, caps.levelCap, caps.specialCap};
        weapons.craft(slot).raiseAiLoadout(int(taken.type), capped);
        burstOnCraft(taken, ship, slot);
    }
}

void App::stepParticles(const glm::vec3 &player)
{
    particles.step(slotPositions(player), simClock);
}

// where each grid slot's craft is (the particles that follow one), the player's in slot 0
std::vector<glm::vec3> App::slotPositions(const glm::vec3 &player) const
{
    std::vector<glm::vec3> positions(kShips, glm::vec3(0.f));
    positions[0] = player;
    for (const auto &drone : race.grid())
    {
        if (drone.slot > 0 && drone.slot < kShips)
        {
            positions[size_t(drone.slot)] = glm::vec3(drone.cur[3]);
        }
    }
    return positions;
}

void App::loadParticleTextures()
{
    renderer.loadParticleTextures();
    if (!weapons.book().hasRecords())
    {
        loadWeapons();
    }
}

