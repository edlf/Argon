// Argon: what a finished lap beats (race_craft_lap 0x3d5a0, Time Attack's 0x429c0) and how FE FASTESTLAP
// shows the records it set (enter 0x20fa0); docs\frontend\options.md "Options sub-pages"
#pragma once
#include <array>

namespace argon::laps
{
constexpr int kTracks = 18;

struct LapTimes
{
    float raceBest;      // +0xd338: starts at the track's record
    float profileBest;   // g_profile + (speed + track x 5) x 4
    float record;        // g_track_records + (speed + track x 5) x 0x18 + 4
};

struct LapVerdict
{
    bool raceBest = false;      // "New lap record" (+0xd32c)
    bool profileBest = false;   // Time Attack: the profile's own best, and the way back through FE FASTESTLAP
    bool record = false;        // the table takes it with the profile's name (0x52ac03[track] = 1)
};

// every mode: a lap under the race's best is its new best, and under the table's time the record; Time Attack only
// looks at the table when the lap also beats the profile's own best
LapVerdict judgeLap(float lap, const LapTimes &before, bool timeAttack);

// FE FASTESTLAP opened after new records (0x20fa0 with +0x4151): it shows the second routes when no first-route track
// (0..7) nor Mars (0x10) has one, but a second route (8..15) or Terraformed Mars (0x11) has
bool opensOnSecondRoutes(const std::array<bool, kTracks> &newRecords);

}  // namespace argon::laps
