// The lap records (g_track_records 0x1bbc34: 18 tracks x 5 speeds of 0x18 bytes, +4 the time, +8 the profile's name;
// index speed + 5 x track), the game's "laptimes" save on the save device (0xc0f90 writes the 0x870 bytes). Reset
// (0x83cc0): every time 999999, every name fe 0x20c "--------". The viewer keeps <saves>\records\laptimes.json
// ({"lapRecords": [{"track", "speed", "time", "name"}]}: track = the game's track id 0..17, speed = class 0..4).
// Shown by Track Records (ui\records.cpp).
#pragma once
#include <array>
#include <string>

constexpr int kLapTableTracks = 18, kLapTableSpeeds = 5;
constexpr float kUnsetLap = 999999.f;

struct LapRecord
{
    float time = kUnsetLap;
    std::u16string name;
    bool set() const
    {
        return time < kUnsetLap;
    }
};

class LapRecordBook
{
  public:
    static constexpr int kTracks = kLapTableTracks, kSpeeds = kLapTableSpeeds;

    void load(const std::string &file);
    void save(const std::string &file) const;
    LapRecord &at(int track, int speed);
    const LapRecord &at(int track, int speed) const;

  private:
    LapRecord records_[kTracks * kSpeeds];
};

// the profile's own best laps (g_profile +0: 18 tracks x 5 speeds, index speed + 5 x track; Time Attack only), in the
// profile JSON's "bestLaps" as the 90 times, comma separated
class ProfileBestLaps
{
  public:
    ProfileBestLaps();
    static ProfileBestLaps read(const std::string &text);   // unset where the text is short

    float at(int track, int speed) const;
    void set(int track, int speed, float time);
    std::string text() const;

  private:
    std::array<float, kLapTableTracks * kLapTableSpeeds> times_;
};
