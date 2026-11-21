// Argon: the race in play as App steps it: who drives and from where, whether it is held, paused or over, and the
// counters and clocks it keeps from one tick to the next. The menus' races and the editor's drive share it.
#pragma once
#include "ui/race_end_page.hpp"

struct RaceSession
{
    bool driving = false;       // the crafts are stepped (the race, or the editor's drive)
    bool simStarted = false;    // the crafts are placed on the grid (App::resetDriver)
    bool playerDrive = false;   // the player's pad drives the slot-0 ship, not the AI
    bool chaseCam = true;       // the drive camera owns the view (else the free camera)
    bool paused = false;
    bool raceWasHeld = false;   // the last tick held the grid (the start's countdown)
    int raceEnd = race_end::kRacing;   // 0x52afa8: racing, or the race-end page up
    int raceEndScenePage = -1;  // the page the race leaves for once its end scene is over
    int hudLaps = 3;            // g_race_laps 0x52cdd8: set as each race loads (setRaceLaps)
    int lapLineCrossings = 0;   // the player's lap-line crossings seen by stepLapLine
    int warningRespawnsSeen = 0;
    float lapRecordSoundAt = -1e9f;   // 0xc3400's clock (+0x220): the last time the lap record sound played
    float turboGrantT = -10.f;        // the effects' clock at the last turbo grant (craft +0x114c, the HUD fan glow)
};
