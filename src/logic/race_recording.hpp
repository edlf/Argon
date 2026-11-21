// Argon: the race recorded in the game's replay format (neon_gamelogic_record_replay 0x3f210 at the race clock's reset,
// 0x3eee0 each frame, stopped by 0x3f2f0 when POSTABLE opens or past 1200 s; docs\replays\README.md): every craft's state
// each race step at the recording's own clock (held by the grid intro), the weapon entries as the weapons act, a shield
// entry for each craft whose shield changed. The last race's recording is kept for View Replay, with where its clock
// started and the first step after the countdown, so the playback moves the effects' clock from the same step.
#pragma once
#include "logic/replay_weapons.hpp"
#include "logic/replay_writer.hpp"

#include <array>
#include <string>
#include <vector>

class RaceRecording
{
  public:
    static constexpr int kCrafts = 16;
    static constexpr float kStep = 1.f / 60.f;
    static constexpr float kLongest = 1200.f;   // 0x1b9790

    // the race's steps against the effects' clock
    struct Timing
    {
        float simClockAtStart = 0;
        int steps = 0, goStep = -1;   // clock steps recorded; the first one after the countdown
    };

    void start(const argon::replay::Info &lineup, float simClock, const std::array<float, kCrafts> &shields);
    // one race step: the clock moves unless the grid intro holds it; `held`: the crafts still wait for GO
    void tick(bool introRunning, bool held);
    void recordFrame(const std::vector<argon::replay::CraftEvent> &crafts);
    void recordWeapon(const argon::replay::WeaponEvent &event);
    // a craft's shield (signal 1) when it changed since last written; knockedOutBy: its killer, else itself
    void recordShield(int slot, float energy, int knockedOutBy);
    void stop();               // the recording kept as the last race's
    void stopWithoutKeeping(); // View Replay: the race's own recording stays the one to view

    bool recording() const;
    bool pastLongest() const;
    float clock() const;
    size_t frames() const;
    std::string weaponsSummary() const;
    const argon::replay::RecordingFiles &lastRace() const;
    const Timing &lastRaceTiming() const;
    bool hasLastRace() const;
    void saveLastRace(const std::string &stem) const;   // <stem>.info / .index / .events; throws with the file on failure

  private:
    argon::replay::RecordingWriter writer_;
    argon::replay::RecordingFiles lastRace_;
    float clock_ = 0;   // 0 at the countdown's start
    Timing timing_, lastRaceTiming_;
    argon::replay::ShotTally shots_;
    std::array<float, kCrafts> shields_{};   // each craft's shield as last written
};
