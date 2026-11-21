// Argon: the game's lap-time text (0x1f860 / 0x1f910): MM:SS.hh; unset (negative, or the 999999 of an empty record)
// "00:00.00" for the lap being driven, else "--:--.--"
#pragma once
#include <string>

enum class LapShown
{
    Current,   // the lap being driven
    Record     // a best or a recorded lap
};

std::u16string lapTimeText(float seconds, LapShown shown);
