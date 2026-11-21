// Argon: the shield bar's fill colour (hud\shield_bar_rules.hpp)
#include "hud/shield_bar_rules.hpp"

namespace argon::hud
{
namespace
{
constexpr uint32_t kYellow = 0xffffd000u, kRed = 0xffff0000u, kWhite = 0xffffffffu;
constexpr float kLowShieldPercent = 0.33f;   // 0x1a3640, compared with the shield in % as it is
constexpr unsigned kBlinkPeriodMask = 0xf, kBlinkHalf = 8;

bool inSecondHalfOfBlink(unsigned vblanks)
{
    return (vblanks & kBlinkPeriodMask) >= kBlinkHalf;
}
}  // namespace

uint32_t shieldFillColour(float shieldPercent, float flashLevel, unsigned vblanks)
{
    if (flashLevel > 0.f && !inSecondHalfOfBlink(vblanks))
    {
        return kWhite;
    }
    if (shieldPercent < kLowShieldPercent && inSecondHalfOfBlink(vblanks))
    {
        return kRed;
    }
    return kYellow;
}

}  // namespace argon::hud
