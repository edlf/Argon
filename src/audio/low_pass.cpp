// Argon: the listener filters, see low_pass.hpp.
#include "audio/low_pass.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace argon
{
namespace
{
constexpr float kPi = 3.14159265f;
constexpr int kLowPassFlags = 0xff00;   // the flags' high byte set: mode 1, the low-pass
constexpr float kLowestCutoff = 30.f, kHighestCutoff = 8000.f, kMostResonanceDb = 22.5f;
constexpr float kFlatQ = 0.7071f;

LowPassSetting readSetting(std::istringstream &in)
{
    int flags = 0, cutoff = 0, resonance = 0;
    std::string rest;
    in >> flags >> cutoff >> resonance;
    std::getline(in, rest);
    LowPassSetting setting;
    setting.on = (flags & kLowPassFlags) == kLowPassFlags;
    setting.cutoffHz = std::clamp(float(cutoff), kLowestCutoff, kHighestCutoff);
    setting.resonanceDb = std::clamp(float(resonance), 0.f, kMostResonanceDb);
    return setting;
}
}  // namespace

ListenerFilters parseListenerFilters(const std::string &text)
{
    ListenerFilters filters;
    std::istringstream in(text);
    std::string name;
    while (in >> name)
    {
        const LowPassSetting setting = readSetting(in);
        if (name == "Internal")
        {
            filters.internal = setting;
        }
        else if (name == "Underwater")
        {
            filters.underwater = setting;
        }
    }
    return filters;
}

// RBJ low-pass; the resonance (dB of peak) as its Q, 0 dB flat
void ResonantLowPass::configure(const LowPassSetting &setting, int sampleRate)
{
    on_ = setting.on;
    if (!on_)
    {
        return;
    }
    const float quality = std::max(kFlatQ, std::pow(10.f, setting.resonanceDb / 20.f));
    const float w0 = 2.f * kPi * std::min(setting.cutoffHz, 0.45f * float(sampleRate)) / float(sampleRate);
    const float alpha = std::sin(w0) / (2.f * quality), cosw = std::cos(w0), a0 = 1.f + alpha;
    b0_ = (1.f - cosw) * 0.5f / a0;
    b1_ = (1.f - cosw) / a0;
    b2_ = b0_;
    a1_ = -2.f * cosw / a0;
    a2_ = (1.f - alpha) / a0;
}

void ResonantLowPass::process(float *samples, size_t frames, size_t channels)
{
    if (!on_)
    {
        return;
    }
    const size_t filtered = std::min(channels, kMaxChannels);
    for (size_t frame = 0; frame < frames; ++frame)
    {
        for (size_t channel = 0; channel < filtered; ++channel)
        {
            std::array<float, 4> &history = state_[channel];
            float &sample = samples[frame * channels + channel];
            const float y = b0_ * sample + b1_ * history[0] + b2_ * history[1] - a1_ * history[2] - a2_ * history[3];
            history = {sample, history[0], y, history[2]};
            sample = y;
        }
    }
}

}  // namespace argon
