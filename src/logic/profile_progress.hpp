// Argon: what FE CHOOSESAVE's "Percentage Complete" counts (0xc03a0, run on every profile the save list
// reads) and FE CHARSELECT's Trophy Level (0x82910); docs\frontend\profiles.md "Profiles"
#pragma once
#include <array>
#include <cstdint>

namespace argon
{
// the Quick Race wins the profile remembers (POSTABLE 0x8ecc7: a Quick Race won at Redshift speed)
enum class RedshiftWin : uint32_t
{
    MartianRoute = 1,       // 0x1bbc34: on Mars' Martian route (track 0x10)
    TerraformedRoute = 2,   // 0x1bbc35: on its Terraformed route (0x11)
    WithFirstEgg = 4,       // 0x1bbc36: with easter egg 0 switched on (any track)
};

struct ProfileProgress
{
    static constexpr int kCharacters = 16, kLevels = 5, kEggs = 8;

    std::array<std::array<bool, kLevels>, kCharacters> trophies{};   // character x level (0x1bb0ef + c x 0xb0)
    uint16_t characters = 0;                                           // unlocked, bit per character (0x1bb0e0)
    uint32_t eggs = 0;                                                 // unlocked, bit per egg (0x1bbc08)
    uint32_t redshiftWins = 0;                                         // RedshiftWin bits

    // 0x82910: the highest level whose trophy the character holds, + 1; 0 without any
    int trophyLevel(int character) const;
    // 0xc03a0: the trophy levels (16 x 0..5), +1 per unlocked character 8..15, +1 per unlocked egg, +1 / +1 / +2 for
    // the Redshift wins: 100 in all
    int percentComplete() const;

  private:
    int unlockedLaterCharacters() const;
    int unlockedEggs() const;
    int redshiftWinPoints() const;
};
}  // namespace argon
