#include "audio/audio_output.hpp"

#include "audio/mix_paths.hpp"
#include "audio/peak_limiter.hpp"
#include "audio/world_effects.hpp"
#include "miniaudio.h"

namespace argon
{
namespace
{
const float kLimiterThreshold = 0.97f;
const float kLimiterReleaseSeconds = 0.05f;
const int kXboxRate = 48000;   // the APU's mix rate
}  // namespace

struct AudioOutput::Mixer
{
    ma_engine engine{};
    PeakLimiter limiter;
    std::unique_ptr<WorldEffects> world;   // released before the engine
    bool hasDevice = false;                 // false: tests mix by renderFrames

    // miniaudio's audio thread, after it has mixed every sound
    static void limitFinalMix(void *mixer, float *frames, ma_uint64 frameCount)
    {
        auto *owner = static_cast<Mixer *>(mixer);
        owner->limiter.process(frames, size_t(frameCount), int(ma_engine_get_channels(&owner->engine)));
    }
};

AudioOutput::AudioOutput() = default;

AudioOutput::~AudioOutput()
{
    if (mixer_)
    {
        mixer_->world.reset();
        ma_engine_uninit(&mixer_->engine);
    }
}

void AudioOutput::open(mix::SpeakerLayout layout)
{
    layout_ = layout;
    start(Device::Played, 0);
}

void AudioOutput::openWithoutDevice(int sampleRate, mix::SpeakerLayout layout)
{
    layout_ = layout;
    start(Device::None, sampleRate);
}

void AudioOutput::start(Device device, int sampleRate)
{
    const bool withDevice = device == Device::Played;
    auto mixer = std::make_unique<Mixer>();
    ma_engine_config config = ma_engine_config_init();
    config.channels = ma_uint32(mix::channelCount(layout_));
    config.sampleRate = ma_uint32(sampleRate ? sampleRate : kXboxRate);
    config.noDevice = withDevice ? MA_FALSE : MA_TRUE;
    config.noAutoStart = MA_TRUE;   // the limiter needs the sample rate first
    config.onProcess = Mixer::limitFinalMix;
    config.pProcessUserData = mixer.get();
    if (ma_engine_init(&config, &mixer->engine) != MA_SUCCESS)
    {
        throw std::runtime_error("no audio output device");
    }
    mixer->limiter = PeakLimiter(kLimiterThreshold, kLimiterReleaseSeconds, int(ma_engine_get_sample_rate(&mixer->engine)));
    auto world = std::make_unique<WorldEffects>();
    if (world->init(&mixer->engine, layout_))
    {
        mixer->world = std::move(world);
    }
    if (withDevice && ma_engine_start(&mixer->engine) != MA_SUCCESS)
    {
        ma_engine_uninit(&mixer->engine);
        throw std::runtime_error("the audio output device did not start");
    }
    mixer->hasDevice = withDevice;
    mixer_ = std::move(mixer);
}

bool AudioOutput::isOpen() const
{
    return mixer_ != nullptr;
}

mix::SpeakerLayout AudioOutput::layout() const
{
    return layout_;
}

void AudioOutput::setMasterVolume(float volume)
{
    if (mixer_)
    {
        ma_engine_set_volume(&mixer_->engine, volume);
    }
}

void AudioOutput::suspend()
{
    if (mixer_ && mixer_->hasDevice)
    {
        ma_engine_stop(&mixer_->engine);
    }
}

void AudioOutput::resume()
{
    if (mixer_ && mixer_->hasDevice)
    {
        ma_engine_start(&mixer_->engine);
    }
}

void AudioOutput::renderFrames(float *interleaved, uint64_t frames)
{
    if (mixer_)
    {
        ma_engine_read_pcm_frames(&mixer_->engine, interleaved, frames, nullptr);
    }
}

ma_engine *AudioOutput::engine()
{
    return mixer_ ? &mixer_->engine : nullptr;
}

ma_sound *AudioOutput::group(Bus bus, VoicePath path)
{
    if (!mixer_ || !mixer_->world)
    {
        return nullptr;
    }
    return mixer_->world->group(bus == Bus::World, path);
}

void AudioOutput::loadHrtf(const hrtf::TableFiles &files)
{
    if (hrtfTables_.empty())
    {
        hrtfTables_ = hrtf::loadTables(files);
    }
}

void AudioOutput::loadKemar(const std::filesystem::path &file)
{
    if (kemarTables_.empty())
    {
        kemarTables_ = hrtf::loadKemar(file);
    }
}

bool AudioOutput::hasKemar() const
{
    return !kemarTables_.empty();
}

void AudioOutput::setHrtfSet(hrtf::Set set)
{
    hrtfSet_ = set;
}

void AudioOutput::setKemarTaps(int taps)
{
    kemarTaps_ = taps;
}

hrtf::Ears AudioOutput::earsFor(const hrtf::Direction &direction) const
{
    if (hrtfSet_ == hrtf::Set::Kemar && !kemarTables_.empty())
    {
        return hrtf::kemarEarsFor(kemarTables_, direction, kemarTaps_);
    }
    return hrtf::earsFor(hrtfTables_, direction);
}

VoiceRouting AudioOutput::routingFor(const hrtf::Direction &direction) const
{
    const bool surround = layout_ == mix::SpeakerLayout::Surround;
    VoiceRouting routing;
    routing.ears = earsFor(surround ? hrtf::mirroredToFront(direction) : direction);
    const float binsGain = mix::gainOf(mix::kHrtfBinsMb);
    for (size_t tap = 0; tap < size_t(routing.ears.taps); ++tap)
    {
        routing.ears.left[tap] *= binsGain;
        routing.ears.right[tap] *= binsGain;
    }
    if (surround)
    {
        routing.frontRear = hrtf::surroundGains(direction);
    }
    return routing;
}

VoiceRouting AudioOutput::routingWithSpeakers(const hrtf::Direction &direction, const mix::SpeakerFlags &speakers) const
{
    VoiceRouting routing = routingFor(direction);
    routing.speakers = mix::speakerShare(speakers);
    return routing;
}

WorldEffects *AudioOutput::worldEffects()
{
    return mixer_ ? mixer_->world.get() : nullptr;
}

}  // namespace argon
