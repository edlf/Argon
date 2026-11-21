// Argon: the AI's trigger as App runs it (the rules: logic\ai_trigger.cpp, docs\weapons\ai.md "AI firing"): which drones
// fire, what each colour holds, firing. The drones (0x58e40 kind: +0x9f9 nemesis, +0x9f8 home circuit; 0x3f3d0 pickup
// caps and shield level by speed class and kind; 0x791f0 the nemesis race, logic\tourney_rules.cpp; its halving,
// logic\weapon_rules.cpp).
// Viewer: the armed record decides (the game reads the one a level up, 0x6e1e0 / 0x6e660, the same weapon in the
// data); a held special fires on any request (0x6d000, Weapons::fireWeapon); locked on = in the player's lock lists or a
// shot's target.
#include "app/weapons.hpp"

#include "logic/race.hpp"

namespace
{
const int kRed = 0, kYellow = 1, kBlue = 2;
const int kShieldClass = 2;
const int kPlayerSlot = 0;
}  // namespace

argon::weapons::DroneKind Weapons::droneKindOf(int slot) const
{
    const int pilot = host_.pilotOf(slot);
    const int player = host_.pilotOf(kPlayerSlot);
    return argon::weapons::droneKind(pilot, player, host_.trackId());
}

argon::weapons::AiWeaponSetup Weapons::aiWeaponSetupOf(int slot) const
{
    return argon::weapons::aiWeaponSetup(host_.speedClass(), droneKindOf(slot));
}

float Weapons::nemesisShareOf(int slot) const
{
    return argon::weapons::nemesisShare(host_.nemesisRace(), slot);
}

void Weapons::updateAiWeapons()
{
    if (!host_.modeHasWeapons() || !weaponBook.hasRecords())
    {
        return;
    }
    for (const auto &drone : race_.grid())
    {
        const bool racing = host_.stillRacing(drone.craft);
        const bool human = drone.player >= 0 && host_.playerDrives();   // a split-screen player fires on their pad
        if (racing && !human && !craftWeapons[drone.slot].knockedOut())
        {
            stepAiTrigger(drone.slot);
        }
    }
}

// 0x6e9e0: the trigger fires what its clocks allow; any fire draws new delays
void Weapons::stepAiTrigger(int slot)
{
    argon::weapons::CraftWeapon &weapon = craftWeapons[size_t(slot)];
    const argon::weapons::AiFire fire = weapon.pullAiTrigger(aiWants(slot));
    if (fire.red)
    {
        fireWeapon(slot, kRed);
    }
    if (fire.blue)
    {
        fireWeapon(slot, kBlue);
    }
    if (fire.yellow)
    {
        fireWeapon(slot, kYellow);
    }
    if (!(fire.red || fire.blue || fire.yellow))
    {
        return;
    }
    weapon.delayAiFire(argon::weapons::aiFireDelay(host_.speedClass(), rand_));
}

// 0x6e8d0 / 0x6e950: the drone's situation this frame, then its wants by what it holds
argon::weapons::AiWants Weapons::aiWants(int slot)
{
    const Found<const argon::GameCraft> me = race_.craftInSlot(slot);
    const bool nemesis = droneKindOf(slot) == argon::weapons::DroneKind::Nemesis;
    const std::vector<int> listed = lockCandidates(slot);
    const std::vector<int> targets = argon::weapons::aiTargets(listed, nemesis);
    const std::optional<float> percentAhead = me ? std::optional<float>(race_.distance(*me) - race_.distance(host_.playerCraft())) : std::nullopt;
    argon::weapons::AiSituation situation;
    situation.engages = argon::weapons::aiEngages(nemesis, percentAhead);
    situation.hasTargets = !targets.empty();
    situation.targetInLine = situation.engages && argon::weapons::targetInLine(slot, targets, host_.crafts());
    situation.threatened = argon::weapons::aiThreatened(argon::weapons::AiThreat{host_.speedClass(), nemesis, lockedOnTo(slot), shieldPercent(slot)});
    situation.holdsForFinishedPlayer = race_.playerFinished() && std::find(listed.begin(), listed.end(), kPlayerSlot) != listed.end();
    return argon::weapons::aiWants(situation, aiArmament(slot));
}

argon::weapons::AiArmament Weapons::aiArmament(int slot) const
{
    return argon::weapons::AiArmament{armedColour(slot, kRed), armedColour(slot, kBlue), armedColour(slot, kYellow)};
}

argon::weapons::ArmedColour Weapons::armedColour(int slot, int colour) const
{
    argon::weapons::ArmedColour armed;
    const WeaponRec *weaponType = pilotWeapon(slot, armedWeaponSlot(slot, colour));
    if (!weaponType)
    {
        return armed;
    }
    armed.armed = true;
    armed.locksOn = weaponType->lockOn;
    armed.shield = weaponType->cls == kShieldClass && weaponType->shieldBonus.shieldDamage > 0.f;
    armed.inFlight = shotInFlight(slot, colour);
    return armed;
}

bool Weapons::lockedOnTo(int slot) const
{
    for (const PlayerLocks &locks : playerLocks)
    {
        if (locks.holds(slot))
        {
            return true;
        }
    }
    for (const Shot &shot : shots_)
    {
        if (shot.target == slot)
        {
            return true;
        }
    }
    return false;
}
