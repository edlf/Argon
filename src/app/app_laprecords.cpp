// Argon: the race's side of the lap records as App runs it (logic\race_lap_records.hpp): the player's laps judged,
// the table written at once when its record falls (not in --shot runs), the record's sound.
#include "app/app.hpp"

// 0xc3400, called while Time Attack's HUD shows "New lap record": SF_GENERIC_FASTESTLAP (newlap; the game's sound 10,
// matched by name) at most every 5 s, not on the results table
void App::playLapRecordSound()
{
    const float kRepeat = 5.f;   // 0x1a3554
    if (simClock - session.lapRecordSoundAt < kRepeat || session.raceEnd != kRacing)
    {
        return;
    }
    session.lapRecordSoundAt = simClock;
    audio.raceSfx().playOnListener(argon::GenericSound::FastestLap);
}

// the player's finished lap (race_craft_lap 0x3d5a0, Time Attack 0x429c0); not in the editor
void App::playerLapRecord(float lap)
{
    if (editor.active() || !(lap > 1.f))
    {
        return;
    }
    const bool timeAttack = &logic() == &modes.timeAttack();
    if (lapRecords.playerLap({lap, picks.loadTrackId, picks.speedClass, timeAttack, sim.laps()}).record)
    {
        lapRecordSaver.storeLapRecords();
    }
}
