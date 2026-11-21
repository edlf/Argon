// Argon: the attract demo's trigger (docs\frontend\intro-title.md "Attract demo"): fe_shell_update 0x306b0 adds the
// frame time to an idle clock (0x439dbc) that any held pad input zeroes (0xb4bfc); past 75 s (0x1a382c) on a page other
// than FE INTROSCREENS, FE MESSAGEBOX or FE OPTIONS CREDITS, fe_shell_start_attract 0x31c10 starts the next of the
// three replays (0x52c8b4: replay1, replay2, replay3, replay1, ...). Pure.
#pragma once

class AttractIdleClock
{
  public:
    static constexpr float kIdleSeconds = 75.f;   // 0x1a382c

    // one front-end frame: true when the demo starts now (the clock then starts again from 0)
    bool step(float dt, bool inputHeld, bool pageAllowsDemo)
    {
        idle = inputHeld ? 0.f : idle + dt;
        if (idle <= threshold || !pageAllowsDemo)
        {
            return false;
        }
        idle = 0.f;
        return true;
    }
    void reset()
    {
        idle = 0.f;
    }
    float seconds() const
    {
        return idle;
    }
    void setIdleSeconds(float seconds)
    {
        threshold = seconds;
    }   // tests: --attract-after

  private:
    float idle = 0.f, threshold = kIdleSeconds;
};

// attract_init 0x40440: 0x52c8b4 moves on 0 -> 1 -> 2 -> 0 each time a demo starts; the file is replay<n + 1>
class AttractReplayCycle
{
  public:
    static constexpr int kReplays = 3;

    int next() const
    {
        return played + 1;
    }
    void advance()
    {
        played = (played + 1) % kReplays;
    }

  private:
    int played = 0;
};
