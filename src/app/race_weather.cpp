// Argon: the race's weather (app\race_weather.hpp; the rain object 0x91890, the internal tripod's sounds 0x5aa10).
#include "app/race_weather.hpp"

#include "audio/audio.hpp"

void RaceWeather::startRace(int trackId, bool alwaysRain)
{
    raining_ = argon::rain::raceRains(trackId, alwaysRain);
    rain_ = argon::rain::Rain{};
    sources_.audio.stopRain();
    soundOn_ = false;
}

// one frame of the rain object at the camera; its velocity is the player's craft's (the view follows it)
void RaceWeather::step(const WeatherStep &frame)
{
    rain_.setWeather({raining_, frame.covered, frame.underWater});
    if (frame.sprayOnLens)
    {
        rain_.splash();
    }
    rain_.step(frame.camera);
    if (frame.soundsRun)
    {
        stepSound(frame.covered);
    }
}

// the internal tripod's update (0x5aa10), which the craft update (0x5dae3) runs every frame whatever the view, starts
// the loop when the player's AI node is open (flag 0x80 clear) and fades it out under a roof (0xc42e0), and plays the
// track's tunnel sound under one (0xc4220): a covered loop's. Neither after the race (the post-race state) nor in the
// attract demo (0x52afac).
void RaceWeather::stepSound(bool covered)
{
    sources_.audio.setTunnel(covered);
    const bool on = raining_ && !covered;
    if (on == soundOn_)
    {
        return;
    }
    soundOn_ = on;
    if (on)
    {
        sources_.audio.playRain(sources_.gameFolder);
    }
    else
    {
        sources_.audio.stopRain();
    }
}
