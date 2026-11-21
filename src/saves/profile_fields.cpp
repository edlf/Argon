// Argon: a profile's lists and flags as its JSON writes them (saves\profile_fields.hpp).
#include "saves/profile_fields.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>

namespace argon
{
namespace
{
const int kTrophyBits = ProfileProgress::kCharacters * ProfileProgress::kLevels;
const size_t kTrophyDigits = 24;
}  // namespace

std::string commaList(std::span<const int> values)
{
    std::string list;
    for (const int value : values)
    {
        list += std::to_string(value) + ",";
    }
    return list;
}

void readCommaList(const std::string &list, std::span<int> values)
{
    for (int &value : values)
    {
        value = 0;
    }
    size_t at = 0;
    for (size_t i = 0; i < values.size() && at < list.size(); ++i)
    {
        size_t end = list.find(',', at);
        if (end == std::string::npos)
        {
            end = list.size();
        }
        values[i] = std::atoi(list.substr(at, end - at).c_str());
        at = end + 1;
    }
}

TrophyTable readTrophies(const std::string &hex)
{
    TrophyTable trophies{};
    if (hex.size() != kTrophyDigits)
    {
        return trophies;
    }
    const uint32_t words[3] = {uint32_t(std::stoul(hex.substr(16, 8), nullptr, 16)),
                               uint32_t(std::stoul(hex.substr(8, 8), nullptr, 16)),
                               uint32_t(std::stoul(hex.substr(0, 8), nullptr, 16))};
    for (int bit = 0; bit < kTrophyBits; ++bit)
    {
        trophies[size_t(bit / ProfileProgress::kLevels)][size_t(bit % ProfileProgress::kLevels)] = (words[bit / 32] >> (bit % 32)) & 1;
    }
    return trophies;
}

std::string trophyText(const TrophyTable &trophies)
{
    uint32_t words[3] = {0, 0, 0};
    for (int bit = 0; bit < kTrophyBits; ++bit)
    {
        if (trophies[size_t(bit / ProfileProgress::kLevels)][size_t(bit % ProfileProgress::kLevels)])
        {
            words[bit / 32] |= 1u << (bit % 32);
        }
    }
    char text[40];
    std::snprintf(text, sizeof text, "%08x%08x%08x", words[2], words[1], words[0]);
    return text;
}

uint32_t flagMask(std::span<const bool> flags)
{
    uint32_t mask = 0;
    for (size_t i = 0; i < flags.size(); ++i)
    {
        if (flags[i])
        {
            mask |= 1u << i;
        }
    }
    return mask;
}

void readFlagMask(uint32_t mask, std::span<bool> flags)
{
    for (size_t i = 0; i < flags.size(); ++i)
    {
        flags[i] = (mask >> i) & 1;
    }
}

namespace
{
void readTournament(const Profile &profile, GameProgress &progress)
{
    const TrophyTable trophies = readTrophies(profile.trophies);
    for (int c = 0; c < 16; ++c)
    {
        for (int l = 0; l < 5; ++l)
        {
            progress.trophies[c][l] = trophies[size_t(c)][size_t(l)];
        }
    }
    readCommaList(profile.tourney, std::span<int>(&progress.tourney[0][0], 16 * 5));
    progress.tourneyLevel = std::clamp(profile.tourneyLevel, 0, 4);
    progress.redshiftWins = profile.redshiftWins;
}

void readScores(const Profile &profile, GameProgress &progress)
{
    readCommaList(profile.cash, progress.cash);
    int levels[16 * 3];
    readCommaList(profile.upgrades, levels);
    for (int i = 0; i < 16 * 3; ++i)
    {
        progress.upgrades[i / 3][i % 3] = uint8_t(std::clamp(levels[i], 0, 7));
    }
    readCommaList(profile.hiTourney, std::span<int>(&progress.hiTourney[0][0], 16 * 18));
    readCommaList(profile.hiQuick, std::span<int>(&progress.hiQuick[0][0], 16 * 18));
}

void readUnlocks(const Profile &profile, GameProgress &progress)
{
    progress.speeds = std::clamp(profile.speeds, 0, 4);
    progress.characters = profile.characters;
    progress.tracks = profile.tracks;
    progress.mars = profile.mars;
    readFlagMask(profile.eggs, progress.eggs);
    readFlagMask(profile.cheats, progress.cheats);
    progress.cheatMenu = profile.cheatMenu;
}

TrophyTable trophyTableOf(const GameProgress &progress)
{
    TrophyTable trophies{};
    for (int c = 0; c < 16; ++c)
    {
        for (int l = 0; l < 5; ++l)
        {
            trophies[size_t(c)][size_t(l)] = progress.trophies[c][l];
        }
    }
    return trophies;
}

void writeScores(const GameProgress &progress, Profile &profile)
{
    int levels[16 * 3];
    for (int i = 0; i < 16 * 3; ++i)
    {
        levels[i] = int(progress.upgrades[i / 3][i % 3]);
    }
    profile.cash = commaList(progress.cash);
    profile.upgrades = commaList(levels);
    profile.hiTourney = commaList(std::span<const int>(&progress.hiTourney[0][0], 16 * 18));
    profile.hiQuick = commaList(std::span<const int>(&progress.hiQuick[0][0], 16 * 18));
}

void writeUnlocks(const GameProgress &progress, Profile &profile)
{
    profile.characters = progress.characters;
    profile.tracks = progress.tracks;
    profile.mars = progress.mars;
    profile.speeds = progress.speeds;
    profile.eggs = flagMask(progress.eggs);
    profile.cheats = flagMask(progress.cheats);
    profile.cheatMenu = progress.cheatMenu;
}

// FE CHEATMENU's cheats 0..2 open what they open in the profile too
void writeCheatEffects(const GameProgress &progress, Profile &profile)
{
    if (progress.cheatsOn[0])
    {
        profile.characters = 0xffff;
    }
    if (progress.cheatsOn[1])
    {
        profile.speeds = 4;
    }
    if (progress.cheatsOn[2])
    {
        profile.tracks = 0xffffffffu;
        profile.mars = true;
    }
}
}  // namespace

void readProgress(const Profile &profile, ProfileUnlocks unlocks, GameProgress &progress)
{
    readTournament(profile, progress);
    readScores(profile, progress);
    readFlagMask(profile.eggsOn, progress.eggsOn);
    readFlagMask(profile.cheatsOn, progress.cheatsOn);
    if (unlocks == ProfileUnlocks::Kept)
    {
        readUnlocks(profile, progress);
    }
}

void writeProgress(const GameProgress &progress, ProfileUnlocks unlocks, Profile &profile)
{
    profile.tourney = commaList(std::span<const int>(&progress.tourney[0][0], 16 * 5));
    profile.trophies = trophyText(trophyTableOf(progress));
    profile.tourneyLevel = progress.tourneyLevel;
    profile.redshiftWins = progress.redshiftWins;
    writeScores(progress, profile);
    profile.eggsOn = flagMask(progress.eggsOn);
    profile.cheatsOn = flagMask(progress.cheatsOn);
    if (unlocks == ProfileUnlocks::Kept)
    {
        writeUnlocks(progress, profile);
    }
    writeCheatEffects(progress, profile);
}

ProfileProgress progressOf(const Profile &profile)
{
    ProfileProgress progress;
    progress.trophies = readTrophies(profile.trophies);
    progress.characters = profile.characters;
    progress.eggs = profile.eggs;
    progress.redshiftWins = profile.redshiftWins;
    return progress;
}

}  // namespace argon
