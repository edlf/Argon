// Argon: the crosstalk canceller, see crosstalk.hpp.
#include "audio/crosstalk.hpp"

#include <algorithm>
#include <cmath>

namespace argon
{
namespace
{
constexpr float kDspRate = 48000.f;
constexpr float kDelaySamples = 11.f;
constexpr float kPole = 0.924f;
constexpr float kFeed = -0.0585f;
}  // namespace

void Crosstalk::configure(int sampleRate)
{
    const size_t delay = size_t(std::max(1.f, std::round(kDelaySamples * float(sampleRate) / kDspRate)));
    lateLeft_.assign(delay, 0.f);
    lateRight_.assign(delay, 0.f);
    at_ = 0;
    filteredLeft_ = filteredRight_ = 0.f;
}

void Crosstalk::process(float *pair, size_t frames, size_t stride)
{
    if (lateLeft_.empty())
    {
        return;
    }
    for (size_t frame = 0; frame < frames; ++frame)
    {
        float *samples = pair + frame * stride;
        filteredLeft_ = kPole * filteredLeft_ + kFeed * lateLeft_[at_];
        filteredRight_ = kPole * filteredRight_ + kFeed * lateRight_[at_];
        const float left = samples[0] + filteredRight_;
        const float right = samples[1] + filteredLeft_;
        lateLeft_[at_] = left;
        lateRight_[at_] = right;
        samples[0] = left;
        samples[1] = right;
        at_ = (at_ + 1) % lateLeft_.size();
    }
}

}  // namespace argon
