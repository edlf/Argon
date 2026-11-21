// Argon: the pause menu's decisions without the drawing (pausemenu_update 0x898e0; docs\frontend\race-end.md "Pause menu"):
// its sounds, the "Saving Race" wait and the controller message of each player (ui\pause.cpp draws and runs them)
#pragma once
#include <array>

namespace menu
{
// sound_play_ui 0xc3ca0 with the race's generic list (audio\generic): 0x17 MENUNAVIGATE, 0x18 MENUSELECT
enum class PauseSound
{
    None,
    Navigate,
    Select
};

// message 9 (A) 0x89bc6: 0x18 after every item but the setting rows (the volumes 7..9 take the default case, no sound)
PauseSound pickSound(bool settingRow);
// messages 7 / 8 (left / right) on a setting row: the entry steps inside its options (a volume's 0..7), no wrap; 0x17
// only when it moved
int steppedOption(int value, bool up, int optionCount);
PauseSound optionSound(int before, int after);

// Continue, Restart, Track Select, Main Menu and the ghost replays with a volume changed (0x584339): +0x1d4 = 1 from the
// clock 0x292d58; the menu takes no message, "Saving Race" (0x520) replaces the hint; after 0.5 s (0x1a3504) the
// profile is written (0xbfac0, once a race: +0x61c), after 1 s (0x1a3454) the box closes and the item's own step runs
class SavingRace
{
  public:
    static constexpr double kSeconds = 1.0;

    void start(double now);
    void stop();
    bool active() const
    {
        return running;
    }
    bool over(double now) const;   // the second has passed

  private:
    bool running = false;
    double startedAt = 0;
};

// pausemenu_update 0x898e0 (0x8a7f0..0x8b0a6) with the race input 0x3bf60: a racer's controller pulled out shows
// "insert on" in that player's HUD (race 0x548 + port) and pauses the race (PAUSE ON, the pause pages kept hidden
// while any player is out or has not pressed START since: 0x52abfc); START on the reconnected pad clears its wait, its
// message goes ("insert off"); once nobody is out or waiting the pause menu opens (PAUSE ON)
class PadWatch
{
  public:
    static constexpr int kPlayers = 4;
    using PerPlayer = std::array<bool, kPlayers>;
    enum class Event
    {
        None,
        Pause,      // a pad went: the race pauses behind the message
        OpenMenu    // the last message gone: the pause menu shows (cursor on Continue)
    };

    void reset();   // a race starts: no message, every pad as yet unseen
    // one frame: each player's pad connected, and START pressed on it this frame
    Event update(const PerPlayer &connected, const PerPlayer &startPressed);
    bool messageShown(int player) const;
    bool holdsMenu() const;   // somebody out or yet to press START: the pause pages stay hidden

  private:
    bool notePulledOut(const PerPlayer &connected);
    void clearWaits(const PerPlayer &connected, const PerPlayer &startPressed);
    bool clearMessages();

    PerPlayer seen{};      // the pad was there once (the keyboard alone never asks)
    PerPlayer shown{};     // +0x61e + player: "insert on" sent
    PerPlayer waiting{};   // 0x52abfc: out since, START not pressed yet
    PerPlayer out{};       // this frame: the pad is missing
};
}  // namespace menu
