// Argon: a race's own clock and its end by time (the 20-minute limit 0x3b920, Tournament and Quick Race: the race clock
// 0x50e430, from the race setup, at 1200 s ends the race for everyone while the player still races) and the player's
// knock-out (craftdeath 0x61fc0: who did it and when; LOSEMENU follows, app_knockout.cpp).
#pragma once

class RaceClock
{
  public:
    static constexpr float kTimeLimit = 1200.f;   // 0x1b9790

    void start(float now)
    {
        startedAt_ = now;
        timedOut_ = false;
    }
    float elapsed(float now) const
    {
        return now - startedAt_;
    }
    void setLimit(float seconds)   // tests: --time-limit
    {
        limit_ = seconds;
    }
    bool pastLimit(float now) const
    {
        return elapsed(now) >= limit_;
    }
    void endByTime()
    {
        timedOut_ = true;
    }
    bool endedByTime() const
    {
        return timedOut_;
    }

  private:
    float limit_ = kTimeLimit;
    float startedAt_ = 0.f;
    bool timedOut_ = false;
};

class PlayerKnockOut
{
  public:
    static constexpr int kNobody = -1;

    void clear()
    {
        by_ = kNobody;
    }
    void knockedOutBy(int slot, float now)
    {
        by_ = slot;
        at_ = now;
    }
    int by() const   // the knocking-out craft's slot, kNobody before
    {
        return by_;
    }
    float secondsSince(float now) const
    {
        return now - at_;
    }

  private:
    int by_ = kNobody;
    float at_ = 0.f;
};
