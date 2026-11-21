// Argon: the warnings the HUD shows about the player's craft (docs\physics\hud.md "HUD", Messages).
// RaceWarnings is the HUD's part of craft_race_info 0x5ce70 (the craft's vtable 0x74, each 60 Hz step): RESET while the
// respawn's timer craft +8 runs (huddata +0x60), WRONG WAY (huddata +0x43) after heading against the route for over a
// second. ShieldLowWarning is the "Shield Low" popup the power-up panel's draw 0x4eb20 posts while the shield is low.
#pragma once
#include <glm/glm.hpp>

namespace argon
{

class RaceWarnings
{
  public:
    struct Step
    {
        bool respawned = false;      // 0x5cb30 put the craft back on the track this step (craft +5 was set)
        bool alive = true;           // craft +0x1145 clear
        glm::vec3 forward{0, 0, 1};  // craft +0x250
        glm::vec3 route{0, 0, 1};    // the next AI node's position - the current one's (0x1b9bc8, next 0xebc11)
    };

    void reset()
    {
        *this = RaceWarnings();
    }
    void step(const Step &s, float dt);

    float resetTime() const
    {
        return resetLeft;
    }   // > 0: RESET, the static overlay's alpha x 255
    bool wrongWay() const
    {
        return headingWrong;
    }

    static constexpr float kResetTime = 0.5f;          // 0x1950a0, set by 0x5cb30
    static constexpr float kWrongWayDot = -0.4f;       // 0x1a36a0: forward . route at or under it counts
    static constexpr float kWrongWayTime = 1.f;        // 0x1a3454: for longer than this

  private:
    void countDownReset(bool respawned, float dt);
    void judgeHeading(const Step &step, float dt);

    float resetLeft = 0;       // craft +8
    float wrongTime = 0;       // craft +0x8e0
    bool headingWrong = false; // craft +0xa73
};

// 0x4eb20: the shield's fraction (huddata +0x44 x 0.01) dropping under 0.33 starts a 1 s timer (+0x1a80); each time it
// runs out while the fraction stays under, "Shield Low" (event kind 13, no points, 0x83f00) and the timer restarts.
// Also there, not ported: the music mode one up on the drop (0xdec40, modes 3..6) and one down on the recovery.
class ShieldLowWarning
{
  public:
    void reset()
    {
        *this = ShieldLowWarning();
    }
    void step(float shieldFraction, float dt);
    bool popupDue() const
    {
        return due;
    }   // this step posts the popup
    bool wentLow() const
    {
        return crossedDown;
    }   // this step dropped under kLow: the music one up (0x4ecc6)
    bool recovered() const
    {
        return crossedUp;
    }   // this step came back to kLow or over: the music one down (0x4eda1)

    static constexpr float kLow = 0.33f;       // 0x1a3640
    static constexpr float kRepeat = 1.f;      // 0x1a3454

  private:
    float previous = 1.f;   // +0x1a7c
    float untilNext = 0;    // +0x1a80 - now
    bool due = false;
    bool crossedDown = false, crossedUp = false;
};

}  // namespace argon
