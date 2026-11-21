// Argon: the pause menu's decisions (ui\pause_rules.hpp)
#include "ui/pause_rules.hpp"

#include <algorithm>

namespace menu
{
PauseSound pickSound(bool settingRow)
{
    return settingRow ? PauseSound::None : PauseSound::Select;
}

int steppedOption(int value, bool up, int optionCount)
{
    return std::clamp(value + (up ? 1 : -1), 0, std::max(optionCount - 1, 0));
}

PauseSound optionSound(int before, int after)
{
    return before == after ? PauseSound::None : PauseSound::Navigate;
}

void SavingRace::start(double now)
{
    running = true;
    startedAt = now;
}

void SavingRace::stop()
{
    running = false;
}

bool SavingRace::over(double now) const
{
    return running && now > startedAt + kSeconds;
}

void PadWatch::reset()
{
    seen = {};
    shown = {};
    waiting = {};
    out = {};
}

PadWatch::Event PadWatch::update(const PerPlayer &connected, const PerPlayer &startPressed)
{
    clearWaits(connected, startPressed);
    if (notePulledOut(connected))
    {
        return Event::Pause;
    }
    const bool cleared = clearMessages();
    return cleared && !holdsMenu() ? Event::OpenMenu : Event::None;
}

bool PadWatch::messageShown(int player) const
{
    return player >= 0 && player < kPlayers && shown[size_t(player)];
}

bool PadWatch::holdsMenu() const
{
    for (int p = 0; p < kPlayers; ++p)
    {
        if (out[size_t(p)] || waiting[size_t(p)])
        {
            return true;
        }
    }
    return false;
}

// 0x8a7f0: a pad missing whose message is not up yet: "insert on", PAUSE ON (0x52afa4 = the player), 0x52abfc = 1
bool PadWatch::notePulledOut(const PerPlayer &connected)
{
    bool pulled = false;
    for (size_t p = 0; p < size_t(kPlayers); ++p)
    {
        out[p] = seen[p] && !connected[p];
        seen[p] = seen[p] || connected[p];
        if (out[p] && !shown[p])
        {
            shown[p] = true;
            waiting[p] = true;
            pulled = true;
        }
    }
    return pulled;
}

// 0x3bf9c: START (+0x89 & 1) on a waiting player's pad, back in its port
void PadWatch::clearWaits(const PerPlayer &connected, const PerPlayer &startPressed)
{
    for (size_t p = 0; p < size_t(kPlayers); ++p)
    {
        if (waiting[p] && connected[p] && startPressed[p])
        {
            waiting[p] = false;
        }
    }
}

// 0x8ad08: "insert off" for each player shown but neither out nor waiting
bool PadWatch::clearMessages()
{
    bool cleared = false;
    for (size_t p = 0; p < size_t(kPlayers); ++p)
    {
        if (shown[p] && !out[p] && !waiting[p])
        {
            shown[p] = false;
            cleared = true;
        }
    }
    return cleared;
}
}  // namespace menu
