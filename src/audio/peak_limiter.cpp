#include "audio/peak_limiter.hpp"

#include <algorithm>
#include <cmath>

namespace argon
{

PeakLimiter::PeakLimiter(float threshold, float releaseSeconds, int sampleRate)
    : threshold_(threshold), releasePerFrame_(std::exp(-1.f / (releaseSeconds * float(sampleRate))))
{
}

void PeakLimiter::process(float *interleaved, size_t frames, int channels)
{
    for (size_t frame = 0; frame < frames; ++frame)
    {
        float *samples = interleaved + frame * size_t(channels);
        float peak = 0.f;
        for (int c = 0; c < channels; ++c)
        {
            peak = std::max(peak, std::abs(samples[c]));
        }
        envelope_ = std::max(peak, envelope_ * releasePerFrame_);
        if (envelope_ <= threshold_)
        {
            continue;
        }
        const float gain = threshold_ / envelope_;
        for (int c = 0; c < channels; ++c)
        {
            samples[c] *= gain;
        }
    }
}

}  // namespace argon
