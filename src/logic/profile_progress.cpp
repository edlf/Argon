// Argon: "Percentage Complete" (0xc03a0) and the Trophy Level (0x82910); logic\profile_progress.hpp
#include "logic/profile_progress.hpp"

namespace argon
{
namespace
{
bool hasBit(uint32_t bits, int bit)
{
    return (bits >> bit) & 1u;
}
}  // namespace

int ProfileProgress::trophyLevel(int character) const
{
    if (character < 0 || character >= kCharacters)
    {
        return 0;
    }
    for (int level = kLevels - 1; level >= 0; --level)
    {
        if (trophies[size_t(character)][size_t(level)])
        {
            return level + 1;
        }
    }
    return 0;
}

int ProfileProgress::percentComplete() const
{
    int points = 0;
    for (int c = 0; c < kCharacters; ++c)
    {
        points += trophyLevel(c);
    }
    return points + unlockedLaterCharacters() + unlockedEggs() + redshiftWinPoints();
}

// 0xc04a0: bits 8..15 of the character mask (the seven originals and Ka Hekili do not count)
int ProfileProgress::unlockedLaterCharacters() const
{
    int count = 0;
    for (int c = 8; c < kCharacters; ++c)
    {
        count += hasBit(characters, c) ? 1 : 0;
    }
    return count;
}

int ProfileProgress::unlockedEggs() const
{
    int count = 0;
    for (int e = 0; e < kEggs; ++e)
    {
        count += hasBit(eggs, e) ? 1 : 0;
    }
    return count;
}

int ProfileProgress::redshiftWinPoints() const
{
    int count = 0;
    count += (redshiftWins & uint32_t(RedshiftWin::MartianRoute)) ? 1 : 0;
    count += (redshiftWins & uint32_t(RedshiftWin::TerraformedRoute)) ? 1 : 0;
    count += (redshiftWins & uint32_t(RedshiftWin::WithFirstEgg)) ? 2 : 0;
    return count;
}
}  // namespace argon
