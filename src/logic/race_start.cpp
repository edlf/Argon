// Argon: the race start's clocks (race_start.hpp).
#include "logic/race_start.hpp"

#include <algorithm>

namespace
{
constexpr float kDigitsOffSeconds = 10.f;
}  // namespace

void RaceStart::startIntro()
{
    introClock_ = 0.f;
    countdownClock_ = -1.f;
}

void RaceStart::startCountdown()
{
    introClock_ = -1.f;
    countdownClock_ = 0.f;
    digitsSounded_ = 0;
}

void RaceStart::stop()
{
    introClock_ = -1.f;
    countdownClock_ = -1.f;
}

RaceStart::Step RaceStart::step(float dt, int racers, bool skipIntro)
{
    Step happened;
    if (introClock_ >= 0.f)
    {
        introClock_ += dt;
        const int shown = racers - 1 - int(introClock_ / kIntroShotSeconds);
        if (shown < 0 || skipIntro)
        {
            startCountdown();
            happened.countdownStarted = true;
        }
    }
    if (countdownClock_ >= 0.f && introClock_ < 0.f)
    {
        countdownClock_ += dt;
        happened.firstDigit = digitsSounded_;
        while (digitsSounded_ < kDigits && countdownClock_ >= digitArrival(digitsSounded_))
        {
            ++digitsSounded_;
        }
        happened.digits = digitsSounded_ - happened.firstDigit;
        if (countdownClock_ > kDigitsOffSeconds)
        {
            countdownClock_ = -1.f;
        }
    }
    return happened;
}

void RaceStart::showIntroAt(float seconds)
{
    introClock_ = seconds;
}

void RaceStart::showCountdownAt(float seconds)
{
    introClock_ = -1.f;
    countdownClock_ = seconds;
    digitsSounded_ = seconds >= kGoSeconds ? kDigits : int(seconds);
}

bool RaceStart::introRunning() const
{
    return introClock_ >= 0.f;
}

float RaceStart::introClock() const
{
    return introClock_;
}

int RaceStart::introCraft(int racers) const
{
    return std::clamp(racers - 1 - int(introClock_ / kIntroShotSeconds), 0, racers - 1);
}

bool RaceStart::held() const
{
    return introClock_ >= 0.f || (countdownClock_ >= 0.f && countdownClock_ < kGoSeconds);
}

bool RaceStart::countdownShown() const
{
    return countdownClock_ >= 0.f && introClock_ < 0.f && countdownClock_ <= kDigitsOffSeconds;
}

float RaceStart::countdownClock() const
{
    return countdownClock_;
}

float RaceStart::digitArrival(int digit)
{
    return float(std::clamp(digit, 0, kDigits - 1) + 1);
}
