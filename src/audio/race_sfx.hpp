// Argon: the race's sounds as the game's sound manager plays them (sound_play_ui 0xc3ca0, docs\audio\README.md "Race
// sounds"): the generic ones by id and each craft's ship sounds, one voice per play. A positional sound sits at a
// craft (following it) or a point, fades by the rolloff curve between its MINDIST and MAXDIST (sound::rolloffGain),
// does not start beyond MAXDIST (0xc3bb0) and pans by the listener's right. Split screen (game mode 4) plays
// everything on the listener.
#pragma once
#include <glm/glm.hpp>
#include <vector>

#include "audio/generic_sounds.hpp"
#include "audio/spatial.hpp"
#include "audio/voice.hpp"

namespace argon
{

class AudioOutput;

class RaceSfx
{
  public:
    // player_pad_input 0x70d92: the turbo button held with turbo to use, held without, or let go
    enum class Turbo
    {
        Pushing,
        Failing,
        Released
    };

    explicit RaceSfx(AudioOutput &output);
    void setBank(GenericSoundBank bank);
    void setShipSounds(std::vector<ShipSounds> bySlot);
    const SoundOptions &options(GenericSound id) const;
    const SoundOptions &shipOptions(ShipSound id, int craft) const;

    void setListener(const spatial::Listener &listener);
    void setCentred(bool on);   // split screen: every sound on the listener
    void placeCraft(int craft, const glm::vec3 &position);

    // a handle for setVolumeMb / setPitch; -1 when nothing started (no sound, no device, out of range)
    int playOnListener(GenericSound id);
    int playAtCraft(GenericSound id, int craft);
    int playAtPoint(GenericSound id, const glm::vec3 &point);
    int playShipSound(ShipSound id, int craft);
    void moveTo(int handle, const glm::vec3 &point);                        // held at the point from now on
    void setDistances(int handle, float minDist, float maxDist);            // IDirectSoundBuffer_SetMin / MaxDistance
    void stopHandle(int handle);
    bool isPlaying(GenericSound id, int craft) const;                     // 0xc3c20
    bool startedWithin(GenericSound id, int craft, float seconds) const;  // 0xc3ad0's repeat delay
    // 0xc4ba0 -> 0xcafd0(0): the voice is released (no longer the craft's sound) and plays on to its end unlooped
    void stop(GenericSound id, int craft);
    void setTurbo(int craft, Turbo turbo);
    void setVolumeMb(int handle, float mb);
    void setPitch(int handle, float ratio);

    void update(float dt, float sfxVolume);
    void pause();
    void resume();
    void stopAll();

  private:
    static constexpr int kNoCraft = -1;
    struct Playing
    {
        int handle = -1;
        int key = 0;   // a GenericSound, or kShipKey + a ShipSound
        int craft = kNoCraft;
        bool atPoint = false;
        glm::vec3 point{0.f};
        float volumeMb = 0.f;
        double startedAt = 0.0;
        bool released = false;   // stopped by stop(): playing out, no longer found by its key
        SoundOptions options;
        Voice voice;
    };
    enum class Start
    {
        Whole,
        HoldingLoop,   // round the file's smpl loop until stopped
        Tail           // once, from the end of the smpl loop
    };
    static constexpr int kShipKey = 1000;

    const SoundClip *clipOf(int key, int craft) const;
    int start(Playing playing, Start how);
    bool startVoice(Playing &playing, const SoundClip &clip, Start how);
    Bus busOf(const SoundOptions &options) const;
    bool isPlayingKey(int key, int craft) const;
    bool holds(const Playing &playing, int key, int craft) const;
    void stopKey(int key, int craft);
    glm::vec3 positionOf(const Playing &playing) const;
    bool inRange(const Playing &playing) const;
    void applyLevels(Playing &playing) const;
    Playing *find(int handle);
    void dropFinished();

    AudioOutput &output_;
    GenericSoundBank bank_;
    std::vector<ShipSounds> ships_;
    spatial::Listener listener_;
    bool centred_ = false;
    std::vector<glm::vec3> craftPositions_;
    std::vector<Playing> playing_;
    int nextHandle_ = 0;
    double clock_ = 0.0;
    float sfxVolume_ = 1.f;
};

}  // namespace argon
