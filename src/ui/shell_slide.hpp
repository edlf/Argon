// Argon: the shell's title and hint slides between pages (fe_shell_set_state 0x316e0; docs\frontend\README.md
// "Page transitions"). A page's enter calls state 1: the title (0x31620) drops in from above and the hints (header
// y 421, footer y 445) move out from the margins by their own width, both in 10 paced steps. Leaving a page calls
// state 4: the title rises back by the same distance in 10 steps and the hints move out by their width + 100 at
// 10 px a step. Each step is one hud element update (0xad850): paced (+4 = 1) moves (target - start) / 10 a frame,
// unpaced moves +0x20 = 10 px a frame; both stop on the target.
#pragma once
#include <algorithm>

namespace menu
{

class ShellSlide
{
  public:
    static constexpr int kSteps = 10;            // +0x20 of the paced elements
    static constexpr float kStepPixels = 10.f;   // +0x20 of the unpaced hints
    static constexpr float kClearance = 100.f;   // 0x1a3450: past the element's own size
    static constexpr float kTitleY = 36.f;       // 0x1a357c

    void enter()
    {
        start(Motion::Entering);
    }   // state 1
    void leave()
    {
        start(Motion::Leaving);
    }    // state 4 (7: the same)
    void step()
    {
        ++ticks;
    }                    // one 60 Hz frame of the elements

    // the title's y for a title `size` wide: from 36 - (size + 100) to 36 entering, back up leaving
    float titleY(float size) const
    {
        const float away = size + kClearance;
        if (motion == Motion::Entering)
        {
            return kTitleY - away * (1.f - pacedFraction());
        }
        if (motion == Motion::Leaving)
        {
            return kTitleY - away * pacedFraction();
        }
        return kTitleY;
    }

    // how far a hint `width` wide sits outside its place (subtract on the left, add on the right)
    float hintOutset(float width) const
    {
        if (motion == Motion::Entering)
        {
            return width * (1.f - pacedFraction());
        }
        if (motion == Motion::Leaving)
        {
            return std::min(kStepPixels * float(ticks), width + kClearance);
        }
        return 0.f;
    }

  private:
    enum class Motion
    {
        Shown,
        Entering,
        Leaving
    };
    void start(Motion kind)
    {
        motion = kind;
        ticks = 0;
    }
    float pacedFraction() const
    {
        return float(std::min(ticks, kSteps)) / float(kSteps);
    }

    Motion motion = Motion::Shown;
    int ticks = 0;
};

}  // namespace menu
