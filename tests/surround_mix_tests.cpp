// Argon: tests for the mix's speaker layouts (audio/audio_output.hpp, audio/world_effects.hpp, audio/voice.hpp): a
// voice of each path through the real miniaudio graph, mixed without a sound card, in stereo and in 5.1. Without the
// HRTF tables a 3D voice's ears pass its sound through, so the levels are the paths' own. Run with test.bat / test.sh.
#include "audio/audio_output.hpp"
#include "audio/voice.hpp"
#include "check.hpp"

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace
{
using argon::test::checks;
using argon::test::failures;
namespace mix = argon::mix;

const int kRate = 48000;
const size_t kFrames = 4800;        // 0.1 s
const size_t kSettledFrame = 4000;  // past the routing's glide
const float kLevel = 0.5f;

std::shared_ptr<argon::Pcm> constantSound(int channels, const std::vector<float> &levels)
{
    auto pcm = std::make_shared<argon::Pcm>();
    pcm->channels = channels;
    pcm->rate = kRate;
    for (size_t frame = 0; frame < kFrames; ++frame)
    {
        for (int channel = 0; channel < channels; ++channel)
        {
            pcm->samples.push_back(int16_t(levels[size_t(channel)] * 32768.f));
        }
    }
    return pcm;
}

// one frame of the mix, settled, with `sound` playing on `bus`
std::vector<float> settledFrame(mix::SpeakerLayout layout, argon::Bus bus, const std::shared_ptr<argon::Pcm> &sound)
{
    argon::AudioOutput output;
    output.openWithoutDevice(kRate, layout);
    argon::Voice voice;
    voice.setBus(bus);
    voice.start(output, sound, argon::Playback::Looped);
    voice.setVolume(1.f);
    voice.startRouting(output.routingFor({}));
    const size_t channels = size_t(mix::channelCount(layout));
    std::vector<float> mixed(kFrames * channels);
    output.renderFrames(mixed.data(), kFrames);
    return std::vector<float>(mixed.begin() + std::ptrdiff_t(kSettledFrame * channels), mixed.begin() + std::ptrdiff_t((kSettledFrame + 1) * channels));
}

bool near(float value, float expected)
{
    return std::abs(value - expected) < 2e-3f;
}

const float kHrtfBins = mix::gainOf(mix::kHrtfBinsMb), kSpeakerVoice = mix::gainOf(mix::kSpeakerFilesMb), kHalfPower = 0.7071f;
// the crosstalk canceller's steady level on a pair with both sides equal: each side takes the other's, 11 samples late,
// through y = 0.924 y - 0.0585 x
const float kCrosstalkOnEqualSides = 1.f / (1.f + 0.0585f / (1.f - 0.924f));

void aMonoSoundOnTheListenerIsFrontOnlyInStereo()
{
    const std::vector<float> frame = settledFrame(mix::SpeakerLayout::Stereo, argon::Bus::Sfx, constantSound(1, {kLevel}));
    const float level = kLevel * kHrtfBins * kCrosstalkOnEqualSides;
    CHECK(near(frame[0], level) && near(frame[1], level));
}

void aMonoSoundOnTheListenerIsHalfFrontHalfRearInFiveOne()
{
    const std::vector<float> frame = settledFrame(mix::SpeakerLayout::Surround, argon::Bus::Sfx, constantSound(1, {kLevel}));
    const float half = kLevel * kHrtfBins * kHalfPower * kCrosstalkOnEqualSides;
    CHECK(near(frame[mix::kFrontLeft], half) && near(frame[mix::kBackRight], half) && frame[mix::kCentre] == 0.f && frame[mix::kLfe] == 0.f);
}

std::shared_ptr<argon::Pcm> centreAndLfeFiles()
{
    auto pcm = constantSound(int(mix::kSpeakerCount), {0.f, 0.f, kLevel, kLevel, 0.f, 0.f});
    pcm->speakerFiles = true;
    return pcm;
}

void speakerFilesKeepTheirChannelsInFiveOne()
{
    const std::vector<float> frame = settledFrame(mix::SpeakerLayout::Surround, argon::Bus::Sfx, centreAndLfeFiles());
    CHECK(near(frame[mix::kCentre], kLevel * kSpeakerVoice) && near(frame[mix::kLfe], kLevel * kSpeakerVoice) && frame[mix::kFrontLeft] == 0.f);
}

void speakerFilesAreDownmixedInStereo()
{
    const std::vector<float> frame = settledFrame(mix::SpeakerLayout::Stereo, argon::Bus::Sfx, centreAndLfeFiles());
    CHECK(near(frame[0], kLevel * kSpeakerVoice * kHalfPower) && near(frame[1], kLevel * kSpeakerVoice * kHalfPower));
}

void mainBusSoundsPlayOnTheFrontPairInFiveOne()
{
    const std::vector<float> frame = settledFrame(mix::SpeakerLayout::Surround, argon::Bus::Main, constantSound(2, {kLevel, -kLevel}));
    CHECK(near(frame[mix::kFrontLeft], kLevel) && near(frame[mix::kFrontRight], -kLevel) && frame[mix::kBackLeft] == 0.f && frame[mix::kCentre] == 0.f);
}
}  // namespace

int main()
{
    aMonoSoundOnTheListenerIsFrontOnlyInStereo();
    aMonoSoundOnTheListenerIsHalfFrontHalfRearInFiveOne();
    speakerFilesKeepTheirChannelsInFiveOne();
    speakerFilesAreDownmixedInStereo();
    mainBusSoundsPlayOnTheFrontPairInFiveOne();
    std::printf("%d checks, %d failed\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
