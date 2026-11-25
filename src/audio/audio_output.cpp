#include "audio/audio_output.hpp"

#include "audio/mix_paths.hpp"
#include "audio/output_dither.hpp"
#include "audio/peak_limiter.hpp"
#include "audio/world_effects.hpp"
#include "miniaudio.h"

namespace argon
{
namespace
{
const float kLimiterThreshold = 0.97f;
const float kLimiterReleaseSeconds = 0.05f;
const int kXboxRate = 48000;   // the APU's mix rate, which the HRTF and crosstalk filters are designed for
const int kCdRate = 44100;     // the game's songs' rate: a device at it plays them unresampled

// a device at 44.1 kHz mixes at its rate, so the songs and the mix skip a 44.1 -> 48 -> 44.1 kHz round trip; at any
// other rate the mix stays at 48 kHz, where the filters are right, and miniaudio converts it once for the device
int mixRateFor(int deviceRate)
{
    return deviceRate == kCdRate ? kCdRate : kXboxRate;
}
}  // namespace

struct AudioOutput::Mixer
{
    ma_engine engine{};
    PeakLimiter limiter;
    OutputDither dither;
    std::unique_ptr<WorldEffects> world;   // released before the engine
    bool hasDevice = false;                 // false: tests mix by renderFrames, undithered so their mixes stay exact

    // miniaudio's audio thread, after it has mixed every sound
    static void limitFinalMix(void *mixer, float *frames, ma_uint64 frameCount)
    {
        auto *owner = static_cast<Mixer *>(mixer);
        const int channels = int(ma_engine_get_channels(&owner->engine));
        owner->limiter.process(frames, size_t(frameCount), channels);
        if (owner->hasDevice)
        {
            owner->dither.process(frames, size_t(frameCount) * size_t(channels));
        }
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

// with a device, it is opened at its own rate first: kept when that is the mix's rate, else opened again at 48 kHz
void AudioOutput::start(Device device, int sampleRate)
{
    const bool withDevice = device == Device::Played;
    auto mixer = std::make_unique<Mixer>();
    initEngine(*mixer, withDevice, withDevice ? 0 : (sampleRate ? sampleRate : kXboxRate));
    const int deviceRate = int(ma_engine_get_sample_rate(&mixer->engine));
    if (withDevice && mixRateFor(deviceRate) != deviceRate)
    {
        ma_engine_uninit(&mixer->engine);
        mixer->engine = {};
        initEngine(*mixer, withDevice, kXboxRate);
    }
    mixer->limiter = PeakLimiter(kLimiterThreshold, kLimiterReleaseSeconds, int(ma_engine_get_sample_rate(&mixer->engine)));
    auto world = std::make_unique<WorldEffects>();
    if (world->init(&mixer->engine, layout_))
    {
        mixer->world = std::move(world);
    }
    mixer->hasDevice = withDevice;
    if (withDevice && ma_engine_start(&mixer->engine) != MA_SUCCESS)
    {
        ma_engine_uninit(&mixer->engine);
        throw std::runtime_error("the audio output device did not start");
    }
    mixer_ = std::move(mixer);
}

// rate 0: the device's own
void AudioOutput::initEngine(Mixer &mixer, bool withDevice, int rate)
{
    ma_engine_config config = ma_engine_config_init();
    config.channels = ma_uint32(mix::channelCount(layout_));
    config.sampleRate = ma_uint32(rate);
    config.noDevice = withDevice ? MA_FALSE : MA_TRUE;
    config.noAutoStart = MA_TRUE;   // the limiter needs the sample rate first
    config.onProcess = Mixer::limitFinalMix;
    config.pProcessUserData = &mixer;
    if (ma_engine_init(&config, &mixer.engine) != MA_SUCCESS)
    {
        throw std::runtime_error("no audio output device");
    }
}

bool AudioOutput::isOpen() const
{
    return mixer_ != nullptr;
}

int AudioOutput::sampleRate() const
{
    return mixer_ ? int(ma_engine_get_sample_rate(&mixer_->engine)) : 0;
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
    routing.ears.delay *= hrtfDelayScale();
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

// the tables' delays are in 48 kHz samples
float AudioOutput::hrtfDelayScale() const
{
    return isOpen() ? float(sampleRate()) / float(kXboxRate) : 1.f;
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
