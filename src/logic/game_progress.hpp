// Argon: the profile block as the game keeps it in memory (0x1bb0e0..0x1bbc36; docs\frontend\profiles.md "Profiles"): what is
// unlocked, the tournaments, the money, upgrades and high scores, the easter eggs and cheats. The profile's JSON holds
// the same (saves\profile_file.hpp, written through saves\profile_fields.hpp); App's pages read and change it here.
#pragma once
#include <cstdint>

namespace argon
{

struct GameProgress
{
    uint16_t characters = 0xffff;    // 0x1bb0e0; --new-profile: 0x007f (the seven original characters)
    uint32_t tracks = 0xffffffffu;   // 0x1bbbfc: bit i track i, bit i + 8 its second route (all unlocked)
    bool mars = true;                // 0x1bbc00
    int speeds = 4;                  // 0x1bb0e4: the highest open speed (the profile's "speeds")
    int tourneyLevel = 0;            // 0x1bbbec: FE TOURNEYTYPE's cursor, the last tournament started
    int tourney[16][5]{};            // 0x1bb0f4: per character and level the races won; -1 = none running (also once won)
    bool trophies[16][5]{};          // 0x1bb0ef + character x 0xb0: the trophies
    uint32_t redshiftWins = 0;       // argon::RedshiftWin bits (0x1bbc34..36)
    int cash[16]{};                  // 0x1bb0e8 + character x 0xb0
    uint8_t upgrades[16][3]{};       // 0x1bb0ec + character x 0xb0: Weapon Strength / Turbo / Shield levels 0..7
    int hiTourney[16][18]{}, hiQuick[16][18]{};   // 0x1bb108 / 0x1bb150 + (character x 0x2c + track) x 4 (HighScoreTable)
    bool eggs[8]{}, eggsOn[8]{};     // easter eggs unlocked (0x1bbc08) / switched on (0x1bbc10)
    bool cheats[7]{}, cheatsOn[7]{}; // cheats unlocked by code (0x1bbc18) / activated (0x1bbc1f)
    bool cheatMenu = false;          // 0x1bbc26: the profile's name started "CHEAT"
};

}  // namespace argon
