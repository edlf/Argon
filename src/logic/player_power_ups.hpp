// Argon: the player's power-ups (the weapon object's levels +0xa4 / +0xa8 / +0xac and its special, 0x6ba30) and the
// caps a pickup raises them to (the Weapon Strength upgrade's, or the editor's own). The AI's are its weapon
// objects' (logic\craft_weapon.hpp).
#pragma once
#include "logic/weapon_rules.hpp"

struct PlayerPowerUps
{
    int levels[3]{};                                     // red, blue, yellow 0..3
    int special = 0;                                     // specials held
    bool specialShown = false;                           // the HUD's special icon
    argon::weapons::PowerLevels caps{3, 3, 3, 1};        // what a pickup raises them to
    bool capsFromUpgrades = true;                        // the caps from Weapon Strength, else the editor's level cap
    int editorLevelCap = 3;

    // the race's start (0x49360): no special, the levels the tests start with (--powerups, else 0)
    void startRace(const int (&startLevels)[3])
    {
        for (int k = 0; k < 3; ++k)
        {
            levels[k] = startLevels[k];
        }
        special = 0;
        specialShown = false;
    }
};
