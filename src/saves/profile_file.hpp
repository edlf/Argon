// Argon: a profile, <saves>\profiles\<name>.json (docs\frontend\profiles.md "Profiles"): one flat JSON object, the bit masks
// as "0x..." strings, the long lists as comma separated strings. The fields mirror the game's profile block.
#pragma once
#include <cstdint>
#include <filesystem>
#include <string>

namespace argon
{

struct Profile
{
    std::string file;
    std::u16string name;
    bool valid = true;
    uint16_t characters = 0x007f; // profile reset 0x83b60: +0x16c, the seven originals
    uint32_t tracks = 0x0000007f; // +0xc88: tracks 0..6 (Draco, the second routes and Mars locked)
    bool mars = false; // +0xc8c
    int speeds = 1; // +0xc90
    int percent = 0; // "Percentage Complete" (0xc03a0), worked out from the rest on load and save
    uint32_t redshiftWins = 0; // argon::RedshiftWin bits (0x1bbc34..36)
    int tourneyLevel = 0; // 0x1bbbec: FE TOURNEYTYPE's cursor, the last tournament started
    uint32_t eggs = 0, eggsOn = 0; // easter eggs unlocked (0x1bbc08) / switched on (0x1bbc10), bit i
    uint32_t cheats = 0, cheatsOn = 0; // cheats unlocked by code (0x1bbc18) / activated (0x1bbc1f), bit i
    bool cheatMenu = false; // 0x1bbc26: the name started "CHEAT"
    bool musicCustom = false; // 0x1bbc27 clear: Music Source = Custom
    std::string playlist; // "soundtrack\\song|..." (the game keeps soundtrack / song ids)
    std::string tourney; // 16 x 5 tournament progress values (0x1bb0f4), -1 = done
    std::string trophies; // 80 bits (character x 5 + level), 24 hex digits
    std::string cash, hiTourney, hiQuick; // 16 / 16 x 18 / 16 x 18 values, comma separated
    std::string upgrades; // 16 x 3 upgrade levels, comma separated
    std::string bestLaps; // the profile's own best laps, 90 values (speed + 5 x track), comma separated
    std::string options; // the Game / Sound Settings: "type=choice;..."
};

// the profile in a file; one that does not parse, or has no name, comes back with valid = false
Profile readProfileFile(const std::filesystem::path &file);
// writes the profile to its file; throws std::runtime_error when the file can't be written
void writeProfileFile(const Profile &profile);

}  // namespace argon
