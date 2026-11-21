// Argon: the rules and writes over the profile's progress (logic\progress_keeper.hpp).
#include "logic/progress_keeper.hpp"

#include "logic/tourney_rules.hpp"

#include <algorithm>

ProgressKeeper::ProgressKeeper(argon::GameProgress &progress, argon::ProfileStore &profiles) : progress_(progress), profiles_(profiles)
{
}

argon::ProfileUnlocks ProgressKeeper::profileUnlocks() const
{
    return unlockAll ? argon::ProfileUnlocks::Ignored : argon::ProfileUnlocks::Kept;
}

void ProgressKeeper::applyUnlockAll()
{
    if (!unlockAll)
    {
        return;
    }
    for (bool &egg : progress_.eggs)
    {
        egg = true;
    }
    for (bool &cheat : progress_.cheats)
    {
        cheat = true;
    }
    progress_.cheatMenu = true;
}

bool ProgressKeeper::anyEgg() const
{
    for (bool egg : progress_.eggs)
    {
        if (egg)
        {
            return true;
        }
    }
    return false;
}

// the highest open speed (0x1bb0e4); outside Tournament at least Amateur (0x34200)
int ProgressKeeper::speedMax() const
{
    return std::clamp(unlockAll ? 4 : std::max(progress_.speeds, 1), 0, 4);
}

int ProgressKeeper::tourneyDone(int level, int character) const
{
    const int done = progress_.tourney[std::clamp(character, 0, 15)][std::clamp(level, 0, 4)];
    return done < 0 ? 0 : done;   // -1: completed (starts again from race 0)
}

// the track of the character's next race at the level (logic\tourney_rules.hpp)
int ProgressKeeper::nextTourneyTrack(int level, int character) const
{
    return argon::tourney::trackOf(level, character, tourneyDone(level, character));
}

bool ProgressKeeper::charUnlocked(int id) const
{
    return (progress_.characters >> (id & 15)) & 1;
}

// what FE CHOOSESAVE's Percentage Complete counts (0xc03a0, logic\profile_progress.hpp)
argon::ProfileProgress ProgressKeeper::profileProgress() const
{
    argon::ProfileProgress shown;
    for (int c = 0; c < 16; ++c)
    {
        for (int l = 0; l < 5; ++l)
        {
            shown.trophies[size_t(c)][size_t(l)] = progress_.trophies[c][l];
        }
    }
    shown.characters = progress_.characters;
    shown.eggs = argon::flagMask(progress_.eggs);
    shown.redshiftWins = progress_.redshiftWins;
    return shown;
}

// the profile JSON's progress fields (saves\profile_fields.cpp writeProgress); nothing when no profile is loaded
void ProgressKeeper::storeProgress()
{
    if (const auto current = profiles_.current())
    {
        argon::Profile &profile = *current;
        argon::writeProgress(progress_, profileUnlocks(), profile);
        profiles_.save(profile);
    }
}

// FE CHEATMENU A on an unlocked cheat 0..4 (0x1a5e0, 0x1bbc1f + i): 0 all characters (0x1bb0e0 = 0xffff), 1 all
// speeds (4), 2 all tracks and Mars, 3 all easter eggs, 4 all 16 characters fully upgraded, unlocked or not (0x1a83b:
// the levels 0x1bb0ec + character x 0xb0 to 7 / 7 / 7; 0x19ed0, a function-table entry at 0x4394d4 that nothing in the
// menus calls, does only the unlocked ones)
void ProgressKeeper::activateCheat(int cheat)
{
    progress_.cheatsOn[cheat] = true;
    switch (cheat)
    {
    case 0:
        progress_.characters = 0xffff;
        break;
    case 1:
        progress_.speeds = 4;
        break;
    case 2:
        progress_.tracks = 0xffffffffu;
        progress_.mars = true;
        break;
    case 3:
        for (bool &egg : progress_.eggs)
        {
            egg = true;
        }
        break;
    case 4:
        for (auto &character : progress_.upgrades)
        {
            for (auto &level : character)
            {
                level = 7;
            }
        }
        break;
    default:
        break;
    }
    storeProgress();
}

void ProgressKeeper::switchCheat(int cheat)
{
    progress_.cheatsOn[cheat] = !progress_.cheatsOn[cheat];
    storeProgress();
}

// POSTABLE (0x8ecc7, the player first and not the attract demo): a Quick Race at Redshift speed won on Mars' Martian
// route (track 0x10) or its Terraformed route (0x11), and one won with easter egg 0 switched on (0x1bbc10)
void ProgressKeeper::recordRedshiftWin(const WonRace &race)
{
    if (!race.quickRace || race.speed != 4 || race.replayed)
    {
        return;
    }
    if (race.track == 0x10)
    {
        progress_.redshiftWins |= uint32_t(argon::RedshiftWin::MartianRoute);
    }
    else if (race.track == 0x11)
    {
        progress_.redshiftWins |= uint32_t(argon::RedshiftWin::TerraformedRoute);
    }
    if (progress_.eggsOn[0])
    {
        progress_.redshiftWins |= uint32_t(argon::RedshiftWin::WithFirstEgg);
    }
    storeProgress();
}
