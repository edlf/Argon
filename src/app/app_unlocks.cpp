// Argon: easter eggs, cheats and speeds unlocked (docs\frontend\cheats.md "Easter Eggs and Cheats").
// The pages: ui\eggs.cpp, ui\speed.cpp.

#include "app/app.hpp"
#include "ui/fe_shell.hpp"

// the race load (0x792f0 with CL = 1): the eggs switched on copied into the race flags, and 0x52abe3 (weapons on) =
// !Weapons Off. The game skips the copy in multiplayer (game mode 4), which the viewer has no race for.
void App::switchOnRaceEggs()
{
    raceEggs.switchOn(progress.eggsOn);
    weaponsOn = !raceEggs.on(Egg::WeaponsOff);
}

// main_menu 0x30af0: every race flag cleared, 0x52abe3 = 1
void App::switchOffRaceEggs()
{
    raceEggs.clear();
    weaponsOn = true;
}
