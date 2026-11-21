#include "logic/camera_shake.hpp"

#include <algorithm>
#include <limits>

namespace argon::shake
{

namespace
{
constexpr int kRandMiddle = 0x3fff;
constexpr float kRandScale = 1.f / 32767.f;   // 0x1a3748
constexpr float kAxisSize = 0.1f;             // 0x1a3508
constexpr float kFadeRate = 2.f;
constexpr float kKnockShake = 0.01f;          // 0x1a355c
constexpr float kJoltKeep = 0.95f;            // 0x1a3620
constexpr float kJoltMinNormalDot = 0.25f;    // 0x1a3518
constexpr float kJoltGain = 0.5f;             // 0x1a3504
constexpr int kJoltRecords = 24;
constexpr float kJoltRoll = -0.003f;          // 0x1a3734
constexpr float kJoltPitch = 0.01f;           // 0x1a355c
constexpr float kJoltLimit = 0.1f;            // 0x1a366c / 0x1a3508

float joltTerm(float along, float scale)
{
    return std::clamp(along * scale, -kJoltLimit, kJoltLimit);
}
}  // namespace

float randomAxis(unsigned randValue)
{
    return float(int(randValue) - kRandMiddle) * kRandScale * kAxisSize;
}

void CameraShake::step(const ShakeFrame &frame)
{
    timer_ -= frame.dt;
    offset_ -= offset_ * (frame.dt * kFadeRate);
    if (!timerRanOut())
    {
        return;
    }
    rearm(frame.hertz);
    offset_ = frame.direction * frame.amplitude;
}

void CameraShake::kick(const glm::vec3 &direction, float knock)
{
    if (knock <= 0.f)
    {
        return;
    }
    offset_ += direction * (knock * kKnockShake);
}

bool CameraShake::timerRanOut() const
{
    return timer_ <= 0.f;
}

void CameraShake::rearm(float hertz)
{
    timer_ = hertz > 0.f ? timer_ + 1.f / hertz : std::numeric_limits<float>::infinity();
    if (timer_ < 0.f)
    {
        timer_ = 0.f;
    }
}

ShakenView shaken(const ShakenView &view, const glm::vec3 &offset)
{
    ShakenView out = view;
    out.up.x += offset.x;
    out.up.z += offset.z;
    out.look.y += offset.y;
    return out;
}

void SubstepContacts::add(const glm::vec3 &momentumChange, float normalDot)
{
    momentumChange_ += momentumChange;
    normalDotSum_ += normalDot;
    ++count_;
}

float SubstepContacts::averageNormalDot() const
{
    return any() ? normalDotSum_ / float(count_) : 0.f;
}

void ContactJolt::beginFrame()
{
    jolt_ *= kJoltKeep;
    records_ = 0;
}

void ContactJolt::addSubstep(const SubstepContacts &contacts)
{
    if (!contacts.any() || records_ >= kJoltRecords)
    {
        return;
    }
    ++records_;
    if (contacts.averageNormalDot() > kJoltMinNormalDot)
    {
        jolt_ += contacts.momentumChange() * kJoltGain;
    }
}

ShakenView jolted(const ShakenView &view, const glm::vec3 &jolt, const CraftAxes &axes)
{
    ShakenView out = view;
    out.up += axes.right * joltTerm(glm::dot(jolt, axes.right), kJoltRoll);
    out.look += axes.up * joltTerm(glm::dot(jolt, axes.forward), kJoltPitch);
    return out;
}

}  // namespace argon::shake
