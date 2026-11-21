// Argon: a track's looping sounds (docs\audio\ships.md "Track sounds") in the race: one voice per emitter sound, its TRIGGER points zones for it: the loudest one drives its gain
// (the game's rolloff from MINDIST to MAXDIST, 0x194f5c, set on every point's voice by 0xc30f0) and its direction.
// (One voice per point played identical loops on top of each other: comb filtering and overload where zones overlap.)
#pragma once
#include <vector>

#include "audio/audio.hpp"
#include "audio/voice.hpp"

namespace argon
{

class AudioOutput;

class TrackLoops
{
  public:
    struct Levels
    {
        float emitters = 1.f;
    };

    explicit TrackLoops(AudioOutput &output) : output_(output)
    {
    }
    void play(const TrackSounds &sounds);   // stops the old track's loops first
    void stop();
    void hold();
    void resume();
    void update(const spatial::Listener &listener, const Levels &levels, float dt);

  private:
    struct EmitterGroup
    {
        Voice voice;
        std::vector<SoundEmitter> points;
        float gain = 0;   // glides to the loudest point's
    };

    struct Loudest
    {
        float gain = 0.f;
        glm::vec3 at{0.f};
    };

    void startEmitters(const TrackSounds &sounds);
    static Loudest loudestPoint(const EmitterGroup &group, const glm::vec3 &listener);

    AudioOutput &output_;
    std::vector<EmitterGroup> emitters_;
};

}  // namespace argon
