// Quantum Redshift audio for the viewer (played through miniaudio: audio_output.hpp, voice.hpp).
//
// Data (see docs/audio/README.md):
//   audio\tracks\<t>\<t>.txt            sound list: "SF_NAME <folder>", TRIGGERSTART, TRIGGER X/Y/Z/MINDIST/MAXDIST ..., TRIGGEREND
//   audio\tracks\<t>\<folder>\*.txt     per-sound options: LOOPS, NOPOSITION, NOREVERB, LEFT/RIGHT/LS/RS (= _L/_R/... files)
//   audio\tracks\<t>\cut_ambient\       5.1 ambience: Amb_N.L/R/C/Ls/Rs/LFE.wav
//   audio\ships\<pilot>\engine\<p>.ini  engine layers: Loop / file / Vol_i speed mB / Pitch_i speed pitch(1/4096 oct)
//   media\music\<t>_music\              music playlist (PCM)
// WAVs are Xbox ADPCM or PCM16 (wav.hpp).
#pragma once
#include "wav.hpp"
#include "audio/hrtf_set.hpp"
#include "audio/music_modes.hpp"
#include "audio/spatial.hpp"
#include "audio/world_effects.hpp"
#include <glm/glm.hpp>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace argon
{

// a sound by its contents: FLAC ("fLaC", dr_flac) and Ogg Vorbis ("OggS", stb_vorbis) for custom music and the converted
// game sounds, else WAV; throws std::runtime_error naming the file and what is wrong
Pcm loadSound(const std::filesystem::path &path);

// a game sound named by its .wav: the converted copy next to it (tools\assets\convert_assets.py: <name>.flac for the music,
// <name>.ogg for the sound effects) when there is one, else the .wav
Pcm loadGameSound(const std::filesystem::path &wav);

struct SoundEmitter
{        // one TRIGGER point of a positional sound
    std::string name, file;
    glm::vec3 pos;
    float minDist, maxDist;
};

// A track's sounds read from disk (any thread; Audio::playTrackSounds starts them on the main thread). Without
// decoding (no audio device) only the emitters and the music list are filled, for the markers.
struct TrackSounds
{
    struct EmitterLoop
    {
        std::vector<size_t> points;
        std::shared_ptr<Pcm> loop;
    };   // points: into emitters
    std::shared_ptr<Pcm> ambient;   // the story scenes' 5.1 cut_ambient on its speaker mixbins (not played in the race)
    std::vector<std::pair<std::string, std::shared_ptr<Pcm>>> oneShots;   // folder, sound (not positional)
    std::vector<SoundEmitter> emitters;
    std::vector<EmitterLoop> emitterLoops;   // one looping voice per emitter sound
    MusicFolder music;   // media\music\<t>_music
    std::vector<std::string> warnings;
};
TrackSounds readTrackSounds(const std::string &game, const std::string &trackAudio, const std::string &musicTrack, bool decode);

// a craft's engine loops (audio\ships\<pilot>\engine + h_engine), read on any thread
struct EngineSounds
{
    struct Layer
    {
        std::vector<glm::vec2> vol, pitch;
        std::shared_ptr<Pcm> loop;
    };   // loop: none if missing
    std::vector<Layer> layers;
    std::vector<std::string> warnings;
};
EngineSounds readEngineSounds(const std::string &game, const std::string &pilot);
// another craft's engine: the normal set only (craft_sound_bind 0xc5700 loads h_engine for the player's craft)
EngineSounds readOtherEngineSounds(const std::string &game, const std::string &pilot);

// audio\generic\<folder> mixed to one stereo one-shot (nullptr if missing), read on any thread
std::shared_ptr<Pcm> readGenericSound(const std::string &game, const std::string &folder);

// a sound folder's files mixed to one stereo buffer: *_L / *_LS left, *_R / *_RS right, any other both
std::shared_ptr<Pcm> readSoundFolder(const std::filesystem::path &dir);

// a loop's click-free seam: its last ~12 ms crossfaded into its start, the loop from there (loopBegin)
void makeLoopable(Pcm &pcm);

class RaceSfx;

class Audio
{
  public:
    Audio();
    ~Audio();
    void init(mix::SpeakerLayout layout);   // throws std::runtime_error without an output device
    bool ok() const
    {
        return ready_;
    }

    void playTrackSounds(TrackSounds sounds);   // stops the old track's voices, starts these (readTrackSounds)
    void loadEngine(const std::string &game, const std::string &pilot);
    void playEngineSounds(EngineSounds sounds);
    // grid slots 1..n's engines (craft_sound_bind 0xc3220), each on its own curve input (otherEngines): at its craft,
    // MINDIST 50, MAXDIST 200 on the rolloff curve, with doppler; split screen puts them on the listener as the
    // player's (0xc5510: in game mode 4 every craft's engine sits on the listener)
    void playOtherEngines(std::vector<EngineSounds> engines);
    void update(float dt, const glm::vec3 &camPos, const glm::vec3 &forward, const glm::vec3 &up);
    void playOneShot(int index);     // index into oneShots()
    // a one-shot (readGenericSound: silver / bronze / gold pickups, newlap, ...); its index, -1 without a device or
    // sound. Add again after playTrackSounds (which clears the one-shots).
    void playCountdown();            // three, two, one, go one second apart
    void playOneShotNamed(const char *name);   // the one-shot of that name (lower case), if the track has it
    int oneShotIndex(const char *name) const;   // -1 when the track has none of that name
    void setTunnel(bool inside);               // the track's tunnel sound under a roof (0xc4220)
    void stopAll();
    RaceSfx &raceSfx();   // the race's generic sounds by id (race_sfx.hpp), mixed at the SFX volume
    void loadListenerFilters(const std::string &game);   // audio\filters.txt (world_effects.hpp)
    void selectListenerFilter(WorldEffects::Filter filter);   // the filter the world's sounds go through
    // where MIT KEMAR's tables are read from (hrtf_kemar.bin, tools\assets\convert_kemar.py); before the first sounds load
    void setKemarFile(const std::filesystem::path &file);
    // Argon Options' HRTF: the 3D voices' next routings take it (MIT KEMAR needs hrtf_kemar.bin, else the Xbox's)
    void setHrtfSet(hrtf::Set set);
    void setKemarTaps(int taps);   // Argon Options' KEMAR Taps (hrtf::kKemarTapChoices): MIT KEMAR's ears cut to that many
    void stopRaceEvents();   // the one-shots, the countdown, turbo and music; the engine and track loops play on
    // the custom music (FE MUSICSELECT "Custom"): replaces the track's music after playTrackSounds, each song a random
    // pick of the playlist (RandomPickOrder, the game's 0xdf150)
    void setMusicList(const std::vector<std::filesystem::path> &files);
    void preview(const std::filesystem::path &file);   // FE LEVELMUSIC "Preview Song": play it now, once
    void stopPreview();
    void holdRaceSounds();   // the pause menu: every voice holds (the engine stops) until resumed
    void resumeRaceSounds();
    // the window lost the focus: all sound (music, menus, movies, race) stops where it is until resumeOutput
    void suspendOutput();
    void resumeOutput();
    // a movie's soundtrack (FE INTROSCREENS): played once, its position the movie's clock
    void playMovie(std::shared_ptr<Pcm> pcm);
    void stopMovie();
    double movieSeconds() const;   // how far the soundtrack has played; -1 when none plays (or it has ended)
    // front end (docs\audio\README.md "Front end"): the UI sounds of audio\frontend\frontend.txt by id (0 SF_UI_SELECT1
    // ui_button1p, 1 BADSELECT1 ui_rollover3, 2 ROLLOVER1 ui_rollover1, 3 SCREENFORWARD ui_xition1_LR, 4 SCREENBACK
    // ui_xition2, 5 GOTOGAME ui_xition3_LR) at the SFX volume; the menu music: mode 1 media\music\frontend_music
    // with the ui_amb3 loop, 2 levelload_music with ui_levelload (the loading screen), 0 off. Independent of the
    // race's music; call updateFrontEnd every frame.
    void loadUi(const std::string &game);
    void playUi(int id);
    // FE OPTIONS SOUND's Engine Volume row (0x2ea60): SF_UI_ENGINETEST (audio\frontend\enginetest, LOOPS 1) at the
    // engine volume with the menu music muted, until stopEngineTest (0x2eb00)
    void playEngineTest(const std::string &game);
    void stopEngineTest();
    // the rain loop (0xc42e0): SF_GENERIC_RAINWINDSHIELD (audio\generic\rain_windshield: its _L / _R pair, LOOPS 1)
    // at the SFX volume until stopRain
    void playRain(const std::string &game);
    void stopRain();
    void setFrontEnd(const std::string &game, int mode);
    void updateFrontEnd();
    // a story scene's ambience (0x9a810 / 0x7fc56 load the track's cut_ambient, 0xca540 loops it at the SFX option
    // less 5 dB; the race never plays it), until stopSceneAmbience (the scene's leave)
    void playSceneAmbience();
    void stopSceneAmbience();
    // 0x32550 on each front-end page's enter: the menus' ui_amb3 loop stops and starts again only when `ambience`
    // (Music Volume at 0, the page not FE TITLE / FE INTROSCREENS); the loading screen's loop is not touched
    void enterFrontEndPage(bool ambience);

    // the race music's mode (docs\audio\music.md "Music"): the game's events change what plays after the current file
    void startRaceMusic();       // the race starts (0x44259 / 0x806fc): on_start.wav, then the segments
    // 0xc8e10: the race music stopped and started again from the given mode (MusicModes::Intro / OnStart), at the
    // setting; the custom music goes on to its next song
    void restartRaceMusic(int mode);
    void playSecondLapMusic();   // the player starts lap 2 (0x6318b): on_start_second_lap.wav, where there is one
    void raiseRaceMusic();       // the final lap, taking the lead, the shield under 33 %: one segment up
    void lowerRaceMusic();       // losing the lead, the shield back over 33 %: one segment down
    void playRaceFinale();       // POSTABLE ON (0x8eb10): the folder's last segment
    // the race end's music fade (audio\music_fade.hpp) as a gain on the race music at the setting (1 = none); back
    // to 1 when the race music stops or another starts (0xdf260 starts a song at the setting)
    void setRaceMusicFade(float gain);

    const std::vector<SoundEmitter> &emitters() const
    {
        return emitters_;
    }
    const std::vector<std::string> &oneShots() const
    {
        return oneShotNames_;
    }
    const std::vector<std::string> &warnings() const
    {
        return warnings_;
    }
    std::string nowPlaying() const;
    int engineLayers() const;

    // settings (read every update): the game's options as gains (default step 4, -400 mB), the ambience and emitters
    // also at the SFX volume, times the viewer's own ambient / emitter sliders; master is the editor's slider (the game
    // has none: 1 plays at its level)
    static constexpr float kDefaultStepGain = 0.631f;
    float master = 1.f, musicVol = kDefaultStepGain, ambientVol = 1.0f, emitterVol = 1.0f, engineVol = kDefaultStepGain,
          sfxVol = kDefaultStepGain;
    bool music = true, ambient = true, emitterSounds = true, engine = true;
    bool muted = false;   // the editor's Mute: everything silent; on when the editor opens
    float throttle = 35.f;           // engine curve input: % of the class top speed (game: 0..500)
    struct CraftEngine
    {
        float throttle = 0.f;   // the curve input, as `throttle`
        glm::vec3 position{0.f}, velocity{0.f};
    };
    std::vector<CraftEngine> otherEngines;   // grid slots 1..n (playOtherEngines)
    glm::vec3 listenerVelocity{0.f};
    bool enginesOnListener = false;          // split screen

  private:
    void playSfx(const std::shared_ptr<Pcm> &pcm);
    void startFrontEndAmbience();
    void loadHrtf(const std::string &game);
    void loadKemar();
    void playFrontEndMusic(const MusicFolder &folder);
    void logSongChange();

    struct Impl;
    std::unique_ptr<Impl> impl_;
    bool ready_ = false;
    int feMode_ = 0;
    bool feAmbWanted_ = false;   // enterFrontEndPage
    hrtf::Set hrtfSet_ = hrtf::Set::Xbox;
    std::filesystem::path kemarFile_;
    std::string loggedSong_;
    std::vector<SoundEmitter> emitters_;
    std::vector<std::string> oneShotNames_, warnings_;
};

} // namespace argon
