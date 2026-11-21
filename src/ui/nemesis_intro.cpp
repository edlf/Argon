// Argon: the Nemesis Challenge intro (loader_draw_frame 0x808e0 at 0x80a12..0x80cb0); ui\nemesis_intro.hpp
#include "ui/nemesis_intro.hpp"

#include <algorithm>
#include <cmath>

namespace menu
{
namespace
{
constexpr int kTurningCopies = 16;
constexpr float kTurnSeconds = 4.f;     // 0x1a3444: the copies settle by then
constexpr float kSettledAt = 2.f;       // 0x1a3534: the yellow text from then
constexpr float kSettledScale = 1.2f;   // 0x1a3684
constexpr uint32_t kRed = 0xff4040u, kYellow = 0xffff40u;

uint32_t withAlpha(uint32_t rgb, uint32_t alpha)
{
    return (alpha << 24) | rgb;
}
}  // namespace

std::vector<IntroText> NemesisIntro::texts(float seconds)
{
    std::vector<IntroText> out;
    addSweeps(out, seconds);
    addTurningCopies(out, seconds);
    addSettled(out, seconds);
    return out;
}

// two huge translucent copies, x 14 from x 800 leftwards at 700 / s (alpha 0x60) and x 20 from 0 rightwards at
// 1000 / s, 100 higher (alpha 0x80)
void NemesisIntro::addSweeps(std::vector<IntroText> &out, float seconds)
{
    out.push_back({800.f - 700.f * seconds, 0.f, 14.f, 0.f, withAlpha(kRed, 0x60)});
    out.push_back({1000.f * seconds, -100.f, 20.f, 0.f, withAlpha(kRed, 0x80)});
}

void NemesisIntro::addTurningCopies(std::vector<IntroText> &out, float seconds)
{
    for (int copy = 0; copy < kTurningCopies; ++copy)
    {
        out.push_back(turningCopy(copy, seconds));
    }
}

// copy i (alpha 16 i) at the centre: with r = 4 - 2t, turned by max(320 r - i max(8 r, 0), 0) deg and scaled
// max((0.4 i + 2.5) (4 - 4t) + 1.4, 1.2); from 4 s all at rest at 1.2
IntroText NemesisIntro::turningCopy(int copy, float seconds)
{
    IntroText text;
    text.colour = withAlpha(kRed, uint32_t(copy) * 16u);
    text.scale = kSettledScale;
    if (seconds >= kTurnSeconds)
    {
        return text;
    }
    const float remaining = kTurnSeconds - 2.f * seconds;
    const float lag = std::max(8.f * remaining, 0.f) * float(copy);
    text.angleDegrees = std::max(320.f * remaining - lag, 0.f);
    text.scale = std::max((0.4f * float(copy) + 2.5f) * (kTurnSeconds - 4.f * seconds) + 1.4f, kSettledScale);
    return text;
}

// from 2 s: the text at rest in yellow (0xffffb040), and an echo of it growing by 20 a second from 1.2 and rising
// (y 240 - 12 x its scale), its alpha 160 x (1 - (t - 2)) down to 0
void NemesisIntro::addSettled(std::vector<IntroText> &out, float seconds)
{
    if (seconds < kSettledAt)
    {
        return;
    }
    out.push_back({320.f, 240.f, kSettledScale, 0.f, 0xffffb040u});
    const float since = seconds - kSettledAt;
    const float fade = std::clamp(1.f - since, 0.f, 1.f);
    const uint32_t alpha = std::min(uint32_t(std::lround(160.f * fade)), 255u);
    const float scale = since * 20.f + kSettledScale;
    out.push_back({320.f, 240.f - scale * 12.f, scale, 0.f, withAlpha(kYellow, alpha)});
}
}  // namespace menu
