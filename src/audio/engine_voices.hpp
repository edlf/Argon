// Argon: the crafts' engine loops (docs\audio\ships.md "Ships"): the player's (engine + h_engine) on the listener and every
// other craft's at its craft (craft_sound_bind 0xc3220), each layer on its volume / pitch curves of the craft's curve
// input (0xc5200).
#pragma once
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "audio/audio.hpp"
#include "audio/hrtf.hpp"
#include "audio/voice.hpp"

namespace argon
{

class AudioOutput;

class EngineVoices
{
  public:
    struct OthersLevel
    {
        float gain = 1.f;
        bool onListener = false;   // split screen: every engine on the listener, as the player's
    };

    explicit EngineVoices(AudioOutput &output) : output_(output)
    {
    }
    void playOwn(EngineSounds &sounds);
    void playOthers(std::vector<EngineSounds> &engines);   // grid slots 1..n
    void stop();                                           // every craft's
    void hold();
    void resume();
    void driveOwn(float curveInput, float gain);
    void driveOthers(const std::vector<Audio::CraftEngine> &crafts, const spatial::Listener &listener, const OthersLevel &level);
    int ownLayers() const
    {
        return int(own_.size());
    }

  private:
    static constexpr float kMcpxBaseRate = 48000.f;   // pitch 0 plays at the MCPX's base rate
    struct Layer
    {
        std::vector<glm::vec2> vol, pitch;
        float fileRate = kMcpxBaseRate;   // the loop's sample rate: SetPitch is absolute (0 = kMcpxBaseRate)
        Voice voice;
    };
    using Layers = std::vector<Layer>;
    struct Mix   // an engine's levels: its gain, the doppler's pitch ratio, its direction's routing
    {
        float gain = 1.f;
        float pitch = 1.f;
        VoiceRouting routing;
    };

    void start(Layers &layers, EngineSounds &sounds);
    Mix otherMix(const Audio::CraftEngine &craft, const spatial::Listener &listener, const OthersLevel &level) const;
    static void drive(Layers &layers, float curveInput, const Mix &mix);

    AudioOutput &output_;
    Layers own_;
    std::vector<Layers> others_;
};

}  // namespace argon
