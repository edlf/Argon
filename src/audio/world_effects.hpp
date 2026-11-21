// Argon: the effects on the game's sounds between their voices and the mix (docs\audio\race-sounds.md "Reverb and filters",
// "Mix levels", "5.1"). Five sound groups feed one node: the 3D voices (their routers' output, in the output's
// channels) with and without the listener filter, the 2D speaker voices (the six speaker mixbins) with and without it,
// and the front pair (music, movies). The filtered ones pass the listener filter (ResonantLowPass), the 3D ones then
// the DSP's crosstalk canceller on each speaker pair it feeds (the front pair; in 5.1 the rear pair too); the speaker
// mixbins are downmixed for stereo (mix::foldSpeakers). No reverb: the game gives every voice an I3DL2 room level of
// -10000 mB, so its DSP reverb never hears a sound. The main thread picks the filter; the audio thread takes the
// change at its next block.
#pragma once
#include <atomic>
#include <memory>
#include <vector>

#include "audio/crosstalk.hpp"
#include "audio/low_pass.hpp"
#include "audio/mix_paths.hpp"

struct ma_engine;
struct ma_sound;   // ma_sound_group is a ma_sound

namespace argon
{

enum class VoicePath;

class WorldEffects
{
  public:
    enum class Filter
    {
        None,
        Internal,
        Underwater
    };

    WorldEffects();
    ~WorldEffects();
    WorldEffects(const WorldEffects &) = delete;
    WorldEffects &operator=(const WorldEffects &) = delete;

    bool init(ma_engine *engine, mix::SpeakerLayout layout);
    // the group a voice plays into (the front pair is never filtered)
    ma_sound *group(bool filtered, VoicePath path);
    // filters.txt's settings, once (later calls are ignored: the audio thread reads them)
    void setFilters(const ListenerFilters &filters);
    void select(Filter filter);

  private:
    struct Graph;
    void applyChanges();
    void mix(const float *const *in, float *out, size_t frames);
    void addVoices3d(const float *filtered, const float *direct, float *out, size_t frames);
    void addSpeakerMixbins(const float *filtered, const float *direct, float *out, size_t frames);
    void addFrontPair(const float *front, float *out, size_t frames) const;
    static void processNode(void *node, const float **in, unsigned *inFrames, float **out, unsigned *outFrames);

    std::unique_ptr<Graph> graph_;
    mix::SpeakerLayout layout_ = mix::SpeakerLayout::Stereo;
    size_t channels_ = 2;
    ListenerFilters filters_;
    std::atomic<bool> filtersReady_{false};
    std::atomic<int> wantedFilter_{0};
    int filter_ = -1;   // what the audio thread has configured
    int sampleRate_ = 48000;
    ResonantLowPass filter3d_, filter2d_;
    Crosstalk frontCrosstalk_, rearCrosstalk_;
    std::vector<float> voices_, mixbins_, folded_;
};

}  // namespace argon
