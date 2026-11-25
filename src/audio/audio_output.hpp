// The sound card: a miniaudio engine mixing every Voice to stereo or 5.1 (the dashboard's Stereo / Dolby Digital,
// fixed when it opens) at the Xbox's 48 kHz, with a PeakLimiter on the final mix. Every sound goes through
// WorldEffects (the listener filter, the crosstalk canceller, the speaker mixbins to the output's channels) on its way;
// the 3D voices take their ears from the game's HRTF tables (hrtf.hpp) or, by choice, MIT KEMAR's (kemar.hpp).
#pragma once
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>

#include "audio/hrtf.hpp"
#include "audio/hrtf_set.hpp"
#include "audio/kemar.hpp"
#include "audio/mix_paths.hpp"
#include "audio/voice_router.hpp"

struct ma_engine;
struct ma_sound;   // ma_sound_group is a ma_sound

namespace argon
{

class WorldEffects;

// what a voice plays: Main straight to the mix (music, movies); the game's sound effects on its voice paths (a mono
// sound on a 3D voice, per-speaker files on 2D voices: mix_paths.hpp), World ones also through the listener filter
enum class Bus
{
    Main,
    Sfx,
    World
};
// what a voice's sound feeds: a 3D voice's router (VoiceRouter, the output's channels), the six speaker mixbins
// (per-speaker files), or the front pair (music, movies, anything on Main: stereo)
enum class VoicePath
{
    Hrtf,
    SpeakerFiles,
    Front
};

class AudioOutput
{
  public:
    AudioOutput();
    ~AudioOutput();
    AudioOutput(const AudioOutput &) = delete;
    AudioOutput &operator=(const AudioOutput &) = delete;

    // throws std::runtime_error without an output device (the viewer then reads sounds without playing them)
    void open(mix::SpeakerLayout layout);
    void openWithoutDevice(int sampleRate, mix::SpeakerLayout layout);   // tests: mixed only by renderFrames
    bool isOpen() const;
    mix::SpeakerLayout layout() const;
    int sampleRate() const;   // the mix's: 44.1 kHz on a 44.1 kHz device, else the Xbox's 48 kHz; 0 until open
    void setMasterVolume(float volume);
    void suspend();   // the device stops: every sound holds where it is, nothing is mixed
    void resume();
    void renderFrames(float *interleaved, uint64_t frames);   // tests: the next frames of the mix (openWithoutDevice)
    ma_engine *engine();           // for Voice::start; null until open
    ma_sound *group(Bus bus, VoicePath path);   // null until open
    WorldEffects *worldEffects();     // null until open
    // once each, throwing std::runtime_error when the file does not hold them
    void loadHrtf(const hrtf::TableFiles &files);         // the game's HRTF tables (hrtf.bin or default.xbe)
    void loadKemar(const std::filesystem::path &file);    // hrtf_kemar.bin (tools\assets\convert_kemar.py)
    bool hasKemar() const;
    // the HRTF the next routings take (the voices glide to it); KEMAR without its tables: the Xbox's
    void setHrtfSet(hrtf::Set set);
    void setKemarTaps(int taps);   // MIT KEMAR's ears cut to this many taps (hrtf::kKemarTapChoices) in the next routings
    // a 3D voice's routing for its source's direction: its HRTF pair (a pass-through until loaded) and, in 5.1, its
    // front / rear share
    VoiceRouting routingFor(const hrtf::Direction &direction) const;
    // a single-file sound's routing: the 3D voice's plus its 2D speaker voices' share (mix::speakerShare)
    VoiceRouting routingWithSpeakers(const hrtf::Direction &direction, const mix::SpeakerFlags &speakers) const;

  private:
    enum class Device
    {
        Played,   // the mix goes to the output device
        None      // mixed only by renderFrames
    };
    void start(Device device, int sampleRate);
    struct Mixer;
    void initEngine(Mixer &mixer, bool withDevice, int rate);
    hrtf::Ears earsFor(const hrtf::Direction &direction) const;
    float hrtfDelayScale() const;

    std::unique_ptr<Mixer> mixer_;
    hrtf::Tables hrtfTables_;
    hrtf::KemarTables kemarTables_;
    hrtf::Set hrtfSet_ = hrtf::Set::Xbox;
    int kemarTaps_ = hrtf::kDefaultKemarTaps;
    mix::SpeakerLayout layout_ = mix::SpeakerLayout::Stereo;
};

}  // namespace argon
