// Argon: the race start's clocks (race_intro_start 0x43d60, race_intro_update 0x43de0; docs\frontend\README.md
// "Race start"). The grid intro (0x52af9d) shows each craft for 2 s, from the end of the craft list to the player, only
// on the first race after a load; a pad's A skips it. Then the countdown: the digits 3 / 2 / 1 / GO arrive at 1 / 2 / 3 /
// 4 s with their sounds (SF_<track>_three .. go), the crafts are held until 4 s and the digits go off past 10 s. The
// camera and the digits' models: app_intro.cpp.
#pragma once

class RaceStart
{
  public:
    static constexpr int kDigits = 4;
    static constexpr float kIntroShotSeconds = 2.f;
    static constexpr float kGoSeconds = 4.f;

    // what one step started
    struct Step
    {
        bool countdownStarted = false;
        int firstDigit = 0, digits = 0;   // the digits whose sounds are due: firstDigit .. firstDigit + digits - 1
    };

    void startIntro();       // the grid intro, then the countdown
    void startCountdown();   // the countdown straight away (Race Again, Restart)
    void stop();             // neither (a replay plays)
    Step step(float dt, int racers, bool skipIntro);

    // tests: the clocks at a moment
    void showIntroAt(float seconds);
    void showCountdownAt(float seconds);

    bool introRunning() const;
    float introClock() const;
    int introCraft(int racers) const;   // the racer the intro shows: racers - 1 - floor(t / 2)
    bool held() const;                  // the crafts wait: the intro, or the countdown before GO
    bool countdownShown() const;
    float countdownClock() const;
    static float digitArrival(int digit);   // 1 / 2 / 3 / 4 s

  private:
    float introClock_ = -1.f, countdownClock_ = -1.f;
    int digitsSounded_ = 0;
};
