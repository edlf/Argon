#include "audio/voice.hpp"

#include <algorithm>
#include <mutex>

#include "audio/audio_output.hpp"
#include "audio/mix_paths.hpp"
#include "miniaudio.h"

namespace argon
{
namespace
{
constexpr ma_uint32 kSoundChannels = 2;   // a mono sound's samples on both, as the router reads them

// a 3D voice's router between its sound and its group: the main thread hands it a new routing, the audio thread takes
// it at its next block (unless the main thread is writing it just then)
struct RouterNode
{
    ma_node_base base;   // first: miniaudio hands the callback this address
    VoiceRouter router;
    std::mutex lock;
    VoiceRouting pending;
    bool hasPending = false, jump = false;

    void hand(const VoiceRouting &routing, bool withoutGlide)
    {
        std::lock_guard<std::mutex> guard(lock);
        pending = routing;
        hasPending = true;
        jump = jump || withoutGlide;
    }

    void takePending()
    {
        std::unique_lock<std::mutex> guard(lock, std::try_to_lock);
        if (!guard.owns_lock() || !hasPending)
        {
            return;
        }
        if (jump)
        {
            router.jumpTo(pending);
        }
        else
        {
            router.setTarget(pending);
        }
        hasPending = jump = false;
    }

    static void process(ma_node *node, const float **in, ma_uint32 *inFrames, float **out, ma_uint32 *outFrames)
    {
        auto *self = static_cast<RouterNode *>(node);
        const ma_uint32 frames = std::min(*inFrames, *outFrames);
        self->takePending();
        self->router.process(in[0], out[0], frames);
        *inFrames = *outFrames = frames;
    }
};
}  // namespace

// miniaudio keeps pointers into every member, so a Sound never moves (Voice holds it on the heap)
struct Voice::Sound
{
    std::shared_ptr<Pcm> pcm;   // the samples the buffer plays from (not copied)
    ma_audio_buffer buffer{};
    ma_sound sound{};
    RouterNode router{};
    bool bufferReady = false, soundReady = false, routerReady = false;   // initialised (only those are released)
    bool held = false;          // stopped by pause()

    Sound() = default;
    Sound(const Sound &) = delete;
    Sound &operator=(const Sound &) = delete;
    ~Sound()
    {
        if (soundReady)
        {
            ma_sound_uninit(&sound);
        }
        if (routerReady)
        {
            ma_node_uninit(&router.base, nullptr);
        }
        if (bufferReady)
        {
            ma_audio_buffer_uninit(&buffer);
        }
    }
};

namespace
{
// a sound played once stops after `fact`'s length; a loop uses every sample
ma_uint64 framesToPlay(const Pcm &pcm, Playback playback)
{
    const size_t all = pcm.samples.size() / size_t(pcm.channels);
    return playback == Playback::Looped || !pcm.playFrames ? all : std::min(all, pcm.playFrames);
}

bool initBuffer(const Pcm &pcm, ma_uint64 frames, ma_audio_buffer &buffer)
{
    ma_audio_buffer_config config = ma_audio_buffer_config_init(ma_format_s16, ma_uint32(pcm.channels), frames, pcm.samples.data(), nullptr);
    config.sampleRate = ma_uint32(pcm.rate);   // miniaudio resamples it to the device's
    return ma_audio_buffer_init(&config, &buffer) == MA_SUCCESS;
}

bool initRouter(ma_engine *engine, mix::SpeakerLayout layout, RouterNode &node)
{
    static ma_node_vtable vtable = {&RouterNode::process, nullptr, 1, 1, 0};
    const ma_uint32 input[1] = {kSoundChannels}, output[1] = {ma_uint32(mix::channelCount(layout))};
    node.router = VoiceRouter(layout);
    ma_node_config config = ma_node_config_init();
    config.vtable = &vtable;
    config.pInputChannels = input;
    config.pOutputChannels = output;
    return ma_node_init(ma_engine_get_node_graph(engine), &config, nullptr, &node.base) == MA_SUCCESS;
}

// what the sound hands its path: the router and the front pair take stereo, the speaker mixbins six channels
ma_uint32 soundChannelsFor(VoicePath path)
{
    return path == VoicePath::SpeakerFiles ? ma_uint32(mix::kSpeakerCount) : kSoundChannels;
}
}  // namespace

Voice::Voice() = default;
Voice::~Voice() = default;
Voice::Voice(Voice &&) noexcept = default;
Voice &Voice::operator=(Voice &&) noexcept = default;

bool Voice::start(AudioOutput &output, std::shared_ptr<Pcm> pcm, Playback playback)
{
    if (!open(output, std::move(pcm), playback))
    {
        return false;
    }
    play();
    return true;
}

bool Voice::startLooping(AudioOutput &output, std::shared_ptr<Pcm> pcm, const Region &loop)
{
    if (!open(output, std::move(pcm), Playback::Looped))
    {
        return false;
    }
    const ma_uint64 frames = sound_->pcm->samples.size() / size_t(sound_->pcm->channels);
    ma_data_source_set_loop_point_in_pcm_frames(&sound_->buffer, std::min<ma_uint64>(loop.begin, frames), std::min<ma_uint64>(loop.end, frames));
    play();
    return true;
}

bool Voice::startFrom(AudioOutput &output, std::shared_ptr<Pcm> pcm, size_t frame)
{
    if (!open(output, std::move(pcm), Playback::Once))
    {
        return false;
    }
    ma_sound_seek_to_pcm_frame(&sound_->sound, frame);
    play();
    return true;
}

// the buffer and sound made, silent and not started yet
bool Voice::open(AudioOutput &output, std::shared_ptr<Pcm> pcm, Playback playback)
{
    stop();
    if (!output.engine() || !pcm || pcm->samples.empty())
    {
        return false;
    }
    auto sound = std::make_unique<Sound>();
    const ma_uint64 frames = framesToPlay(*pcm, playback);
    sound->bufferReady = initBuffer(*pcm, frames, sound->buffer);
    if (!sound->bufferReady)
    {
        return false;
    }
    const VoicePath path = pathOf(*pcm);
    ma_sound *group = output.group(bus_, path);
    if (!group)
    {
        return false;
    }
    if (path == VoicePath::Hrtf)
    {
        sound->routerReady = initRouter(output.engine(), output.layout(), sound->router);
        if (!sound->routerReady)
        {
            return false;
        }
        sound->router.hand(output.routingFor({}), true);
        ma_node_attach_output_bus(&sound->router.base, 0, group, 0);
    }
    ma_sound_config config = ma_sound_config_init_2(output.engine());
    config.pDataSource = &sound->buffer;
    config.flags = MA_SOUND_FLAG_NO_SPATIALIZATION;
    config.channelsOut = soundChannelsFor(path);
    config.pInitialAttachment = path == VoicePath::Hrtf ? static_cast<ma_node *>(&sound->router.base) : static_cast<ma_node *>(group);
    sound->soundReady = ma_sound_init_ex(output.engine(), &config, &sound->sound) == MA_SUCCESS;
    if (!sound->soundReady)
    {
        return false;
    }
    if (playback == Playback::Looped)
    {
        ma_data_source_set_loop_point_in_pcm_frames(&sound->buffer, pcm->loopBegin, frames);
        ma_sound_set_looping(&sound->sound, MA_TRUE);
    }
    sound->pcm = std::move(pcm);
    sound_ = std::move(sound);
    return true;
}

void Voice::play()
{
    left_ = right_ = 1.f;
    volume_ = 0.f;
    applyLevels();
    ma_sound_start(&sound_->sound);
}

void Voice::stop()
{
    sound_.reset();
}

void Voice::release()
{
    if (sound_)
    {
        ma_sound_set_looping(&sound_->sound, MA_FALSE);
    }
}

bool Voice::active() const
{
    return sound_ != nullptr;
}

bool Voice::finished() const
{
    return !sound_ || ma_sound_at_end(&sound_->sound);
}

void Voice::setBus(Bus bus)
{
    bus_ = bus;
}

// mix_paths.hpp: a game sound of one file plays on a 3D voice, one of per-speaker files on 2D voices; Main is the front
// pair
VoicePath Voice::pathOf(const Pcm &pcm) const
{
    if (bus_ == Bus::Main)
    {
        return VoicePath::Front;
    }
    return pcm.speakerFiles ? VoicePath::SpeakerFiles : VoicePath::Hrtf;
}

void Voice::setRouting(const VoiceRouting &routing)
{
    if (sound_ && sound_->routerReady)
    {
        sound_->router.hand(routing, false);
    }
}

void Voice::startRouting(const VoiceRouting &routing)
{
    if (sound_ && sound_->routerReady)
    {
        sound_->router.hand(routing, true);
    }
}

void Voice::setGains(float left, float right)
{
    left_ = left;
    right_ = right;
    applyLevels();
}

void Voice::setVolume(float volume)
{
    volume_ = volume;
    applyLevels();
}

void Voice::setPitch(float ratio)
{
    if (sound_)
    {
        ma_sound_set_pitch(&sound_->sound, ratio);
    }
}

void Voice::pause()
{
    if (!sound_ || sound_->held || !ma_sound_is_playing(&sound_->sound))
    {
        return;
    }
    ma_sound_stop(&sound_->sound);
    sound_->held = true;
}

void Voice::resume()
{
    if (!sound_ || !sound_->held)
    {
        return;
    }
    ma_sound_start(&sound_->sound);
    sound_->held = false;
}

double Voice::secondsPlayed() const
{
    if (finished())
    {
        return -1.0;
    }
    ma_uint64 cursor = 0;
    ma_sound_get_cursor_in_pcm_frames(&sound_->sound, &cursor);
    return double(cursor) / double(sound_->pcm->rate);
}

// the louder side at full gain less the sound's voice path headroom, the other turned down by miniaudio's balance pan;
// a mix stored scaled down (Pcm::storedScale) plays at its own level
void Voice::applyLevels()
{
    if (!sound_)
    {
        return;
    }
    const float louder = std::max(left_, right_);
    const Pcm &pcm = *sound_->pcm;
    const bool speakers = pathOf(pcm) == VoicePath::SpeakerFiles;
    const float path = mix::gainOf(speakers ? mix::kSpeakerFilesMb : mix::kVoice3dMb);
    ma_sound_set_volume(&sound_->sound, volume_ * louder * path / pcm.storedScale);
    if (louder <= 0.f)
    {
        return;
    }
    ma_sound_set_pan(&sound_->sound, left_ >= right_ ? right_ / left_ - 1.f : 1.f - left_ / right_);
}

}  // namespace argon
