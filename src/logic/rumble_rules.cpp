// Argon: a craft's controller rumble rules (rumble_rules.hpp).
#include "logic/rumble_rules.hpp"

#include <algorithm>
#include <cmath>

namespace argon::rumble
{

namespace
{
constexpr float kKnockScale = 1000.f, kKnockMax = 65000.f;                 // 0x1a34d8, 0x1a3740
constexpr float kCompressionStart = 2.2f, kCompressionBase = 1.5f;         // 0x1a3730, 0x1a3688
constexpr float kCompressionScale = -25.f;                                 // 0x1a3738, subtracted
constexpr float kFadeSeconds = 3.f, kFadeSteps = 33.f;                     // 0x1a351c, 0x1a39b8
constexpr uint16_t kFadeStepMotor = 300;
constexpr uint16_t kFirstMotorMin = 0x4000, kSecondMotorMin = 0x2000;

uint16_t gated(uint16_t speed, uint16_t minimum)
{
    return speed > minimum ? speed : uint16_t(0);
}
}  // namespace

uint16_t motorWord(float value)
{
    return uint16_t(std::llrint(value));
}

float decayed(float value, float rate, float dt)
{
    return value - dt * value * rate;
}

float knockMotor(float knock)
{
    return std::clamp(knock * kKnockScale, 0.f, kKnockMax);
}

uint16_t withCompression(uint16_t secondWord, float compression, float speed)
{
    if (compression <= kCompressionStart)
    {
        return secondWord;
    }
    return uint16_t(secondWord - motorWord((compression - kCompressionBase) * speed * kCompressionScale));
}

Motors knockedOutFade(float secondsOut)
{
    if (secondsOut >= kFadeSeconds)
    {
        return {};
    }
    const uint16_t speed = uint16_t(motorWord((kFadeSeconds - secondsOut) * kFadeSteps) * kFadeStepMotor);
    return {speed, speed};
}

Motors padMotors(Motors requested)
{
    return {gated(requested.first, kFirstMotorMin), gated(requested.second, kSecondMotorMin)};
}

Motors playerMotors(const PadState &pad, Motors craft)
{
    if (!pad.vibrationOn)
    {
        return {};
    }
    if (pad.knockedOut)
    {
        return knockedOutFade(pad.secondsOut);
    }
    return pad.flying ? craft : Motors{};
}

}  // namespace argon::rumble
