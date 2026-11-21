// Argon: the grid slots as the scene draws them. The race itself (the drones, the standings):
// logic\race.cpp.
#include "app/app.hpp"
#include "logic/grid_starts.hpp"

// the game only builds the crafts that race (0x79250: the player, then the drones), so the start slots past them
// stay empty; the editor shows a ship on every slot (the grid's markings)
bool App::slotShown(int slot) const
{
    if (slot == 0 && replayRace.hidesPlayerCraft())
    {
        return false;   // the pause menu's ghost replay (0x43770): the player's craft +0x1129
    }
    return editor.active() || race.slotRacing(slot);
}

