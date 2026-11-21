// One sound playing on the AudioOutput: a Pcm buffer played once or looped (over [loopBegin, end)), with left / right
// gains, a volume and a pitch. Each final channel level is volume x that side's gain x its voice path's headroom; a game
// sound of one file goes through the HRTF of a 3D voice (mix_paths.hpp).
#pragma once
#include <memory>

#include "audio/audio_output.hpp"
#include "audio/wav.hpp"

namespace argon
{

// a sound played once, or looped round [loopBegin, end) until stopped
enum class Playback
{
    Once,
    Looped
};

class Voice
{
  public:
    Voice();
    ~Voice();
    Voice(Voice &&) noexcept;
    Voice &operator=(Voice &&) noexcept;

    struct Region
    {
        size_t begin = 0, end = 0;   // frames, end exclusive
    };
    // starts silent (volume 0); false without an output device or when miniaudio refuses the buffer
    bool start(AudioOutput &output, std::shared_ptr<Pcm> pcm, Playback playback);
    // from the start, then round `loop` until stopped (a held sound's sampler loop)
    bool startLooping(AudioOutput &output, std::shared_ptr<Pcm> pcm, const Region &loop);
    // once, from `frame` (a sound's tail after its loop)
    bool startFrom(AudioOutput &output, std::shared_ptr<Pcm> pcm, size_t frame);
    void stop();
    void release();           // a looping voice plays on to its end (DirectSound Play without DSBPLAY_LOOPING)
    bool active() const;     // started and not stopped
    bool finished() const;    // not active, or played to its end
    void setBus(Bus bus);   // where the next start mixes (Main until set)
    // a 3D voice's routing for its source's direction (AudioOutput::routingFor); a voice starts on the listener's
    void setRouting(const VoiceRouting &routing);
    // a just started voice's routing, without the glide (a non-positional sound's, AudioOutput::routingWithSpeakers)
    void startRouting(const VoiceRouting &routing);
    void setGains(float left, float right);
    void setVolume(float volume);
    void setPitch(float ratio);
    void pause();
    void resume();            // only a voice that pause() held
    double secondsPlayed() const;   // -1 when not active or played to its end

  private:
    bool open(AudioOutput &output, std::shared_ptr<Pcm> pcm, Playback playback);
    VoicePath pathOf(const Pcm &pcm) const;
    void play();
    void applyLevels();
    struct Sound;
    std::unique_ptr<Sound> sound_;
    float left_ = 1.f, right_ = 1.f, volume_ = 0.f;
    Bus bus_ = Bus::Main;
};

}  // namespace argon
