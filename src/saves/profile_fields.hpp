// Argon: how a profile's lists and flags are written in its JSON (saves\profile_file.hpp): comma separated values, the
// trophies' 80 bits as 24 hex digits (highest word first), flag arrays as bit masks; and the progress a profile
// shows (FE CHOOSESAVE's Percentage Complete, 0xc03a0, from its own data).
#pragma once
#include "logic/game_progress.hpp"
#include "logic/profile_progress.hpp"
#include "saves/profile_file.hpp"

#include <cstdint>
#include <span>
#include <string>

namespace argon
{

using TrophyTable = std::array<std::array<bool, ProfileProgress::kLevels>, ProfileProgress::kCharacters>;

// "1,2,3," (each value followed by a comma, as the profiles have always been written)
std::string commaList(std::span<const int> values);
// the list into `values` in order, 0 where it is short
void readCommaList(const std::string &list, std::span<int> values);

// 24 hex digits; anything else reads as no trophies
TrophyTable readTrophies(const std::string &hex);
std::string trophyText(const TrophyTable &trophies);

// bit i set for flag i
uint32_t flagMask(std::span<const bool> flags);
void readFlagMask(uint32_t mask, std::span<bool> flags);

// --unlock-all opens everything without it reaching the profile: the profile's unlocks (characters, tracks, Mars,
// speeds, eggs, cheats, the cheat menu) are neither read nor written; the switches and the cheats' effects still are
enum class ProfileUnlocks
{
    Kept,      // read and written with the rest
    Ignored    // --unlock-all
};
// the profile block from the profile (0x83b60's fields; the upgrade levels kept to 0..7, the speeds to 0..4, the
// tournament level to 0..4)
void readProgress(const Profile &profile, ProfileUnlocks unlocks, GameProgress &progress);
// the profile block into the profile; an activated cheat 0 / 1 / 2 leaves its unlocks in it (all characters, all
// speeds, all tracks and Mars)
void writeProgress(const GameProgress &progress, ProfileUnlocks unlocks, Profile &profile);

// 0xc03a0 reads each save's own data: the trophies, the characters, the easter eggs and the Redshift wins
ProfileProgress progressOf(const Profile &profile);

}  // namespace argon
