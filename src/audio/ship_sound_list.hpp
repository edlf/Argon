// Argon: a pilot's ship sound list, audio\ships\<pilot>\<pilot>.txt (docs\audio\ships.md "Ships"): one
// "SF_SHIP_<EVENT> <folder>" per line, separated by spaces or tabs. The folder names differ per pilot
// (darius: do_turbo_comp, do_mstart, ...).
#pragma once
#include <istream>
#include <map>
#include <string>

namespace argon
{

// the event name -> its folder; lines without both are skipped
std::map<std::string, std::string> readShipSoundList(std::istream &list);

}  // namespace argon
