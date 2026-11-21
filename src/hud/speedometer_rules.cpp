// Argon: the speedometer's second needle (hud\speedometer_rules.hpp)
#include "hud/speedometer_rules.hpp"

#include <algorithm>

namespace argon::hud
{
namespace
{
constexpr float kDegreesPerGauge = 1.441f, kJitterShare = 0.1f, kDrawRange = 255.f, kDialEnd = 144.1f;
}  // namespace

float secondNeedleTarget(float throttleGauge, const NeedleJitterDraws &draws)
{
    const float jitter = float(draws.raise - draws.lower) / kDrawRange * throttleGauge * kJitterShare;
    return std::clamp(throttleGauge * kDegreesPerGauge + jitter, 0.f, kDialEnd);
}

}  // namespace argon::hud
