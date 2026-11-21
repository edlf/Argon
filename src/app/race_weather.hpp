// Argon: the race's weather (docs\effects\rain.md "Rain"): whether the race rains (the race load's flag, 0x800b3), the
// rain object (logic\rain.hpp, stepped with the 60 Hz race at the camera: 0x91890), and its loop and the tunnel
// sound (0x5aa10). What the frame is like at the camera comes in a WeatherStep; App works it out from the race.
#pragma once
#include <string>

#include "logic/rain.hpp"

namespace argon
{
class Audio;
}

struct WeatherSources
{
    argon::Audio &audio;              // the rain loop and the tunnel sound
    const std::string &gameFolder;    // the rain loop's file
};

// one 60 Hz frame at the camera
struct WeatherStep
{
    bool covered = false;      // the player's craft's AI node is inside (env_inside): a roof over it
    bool underWater = false;   // the eye below the water's surface
    bool sprayOnLens = false;  // the wake's spray within 2 m of the eye (0xbcfa0)
    bool soundsRun = false;    // the race racing and not the attract demo: the loop and the tunnel follow it
    argon::rain::RainCamera camera;
};

class RaceWeather
{
  public:
    explicit RaceWeather(const WeatherSources &sources) : sources_(sources)
    {
    }
    // the race load (0x800b3) sets the rain flag; the rain object starts dry (0x90d60), its sound off
    void startRace(int trackId, bool alwaysRain);
    void step(const WeatherStep &frame);
    const argon::rain::Rain &rain() const
    {
        return rain_;
    }

  private:
    void stepSound(bool covered);

    WeatherSources sources_;
    argon::rain::Rain rain_;
    bool raining_ = false;   // the race rains on Kalau, alt_bracken or with Always Rain (0x52afa2)
    bool soundOn_ = false;
};
