// Argon: the race's results as the game fixes them at the player's finish (docs\physics\standings.md "Standings"): the finish
// order (0x596db0 +0x514) and POSTABLE's rows (0x8f140), written once, on the frame POSTABLE ON arrives. Pure.
#pragma once
#include <vector>

namespace argon::results
{

constexpr float kNoTime = 999999.f;   // 0x1a3578: no lap yet
constexpr float kPlaceGap = 1.42f;    // 0x1a385c: a place's race time at least the one ahead's + this

// a racer's race slot (0x5972f8 + racer x 0x88) as 0x8f140 reads it
struct RacerProgress
{
    int lapsDone = 0;               // +0x10 - 1 (the lap counter starts at 1)
    float lapFraction = 0.f;        // +0 / 100: the share of the current lap done
    float lapTimeSoFar = 0.f;       // +0x50
    float finishedLapsTime = 0.f;   // +0x60: the laps done (it stops growing at the finish)
    float bestLap = kNoTime;        // +0x58
};

struct RowTimes
{
    float raceTime = 0.f, lapTime = kNoTime;
};

// past the line: its own times; still racing: the current lap's pace p = lap time so far / fraction carried over the
// laps left (no lap done yet: p x laps, and p is the lap column). No progress yet (the game divides by it): the time so far
RowTimes rowTimes(const RacerProgress &racer, int raceLaps);

// 0x8f23e: down the finish order, a race time below the one ahead's becomes that one's + kPlaceGap
void keepPlacesInOrder(std::vector<float> &raceTimesInOrder);

// 0x63850 / 0x638b0 / 0x3f320: the order fills from the front with the finishers, then (at the player's finish or the
// time limit) the racers still racing by their place; the knocked out fill it from the back, the first knocked out
// last. A racer already in it keeps its place.
std::vector<int> finishOrder(const std::vector<int> &finished, const std::vector<int> &racingByPlace, const std::vector<int> &knockedOutFirstToLast);

// 0x8f2e2: "%d: %s" unless the racer is out of the race (craft +0x1145) without having finished
constexpr bool showsPlaceNumber(bool outOfRace, bool finished)
{
    return !outOfRace || finished;
}

}  // namespace argon::results
