// Argon: the game's lap-time text (ui\lap_time_text.hpp)
#include "ui/lap_time_text.hpp"

#include "files/string_table.hpp"

#include <cstdio>

std::u16string lapTimeText(float seconds, LapShown shown)
{
    if (!(seconds >= 0.f) || seconds >= 999999.f)
    {
        return argon::widenLatin1(shown == LapShown::Current ? "00:00.00" : "--:--.--");
    }
    const int minutes = int(seconds / 60.f), whole = int(seconds - minutes * 60.f), hundredths = int((seconds - minutes * 60.f - whole) * 100.f);
    char text[32];
    std::snprintf(text, sizeof text, "%02d:%02d.%02d", minutes, whole, hundredths);
    return argon::widenLatin1(text);
}
