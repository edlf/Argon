#include "audio.hpp"
#include "files/game_path.hpp"

#include "audio/audio_output.hpp"
#include "audio/engine_voices.hpp"
#include "audio/loop_voice.hpp"
#include "audio/game_sound_files.hpp"
#include "audio/mix_paths.hpp"
#include "audio/music_sequence.hpp"
#include "audio/race_sfx.hpp"
#include "audio/sound_reader.hpp"
#include "audio/song_files.hpp"
#include "audio/track_loops.hpp"
#include "audio/world_effects.hpp"
#include "audio/voice.hpp"

#include <algorithm>
#include <random>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <future>
#include <map>
#include <sstream>

namespace fs = std::filesystem;

#include "dr_flac.h"      // custom music decoders (implementations in codecs.c)

namespace argon
{

// ------------------------------------------------------------------ the voices (miniaudio: AudioOutput, Voice)
struct Audio::Impl
{
    AudioOutput output;   // first: destroyed after every voice
    SoundReader reader;
    // the music's modes (0x4406f0; the game has one music object for the race and the front end): before the sequences
    MusicModes raceMusicModes{MusicFolder{}}, frontEndMusicModes{MusicFolder{}};
    RandomPickOrder customMusicOrder;   // the custom soundtrack's songs in a race
    float raceMusicFade = 1.f;   // Audio::setRaceMusicFade

    TrackLoops trackLoops{output};       // the track's emitters
    std::shared_ptr<Pcm> sceneAmbience;   // the track's cut_ambient, for its story scenes
    Voice sceneAmbienceVoice;
    EngineVoices engines{output};        // the player's and every other craft's engine
    std::vector<std::shared_ptr<Pcm>> ui;          // front-end UI sounds by id
    MusicSequence frontEndMusic{output, openSong};   // frontend_music / levelload_music, in order
    Voice feAmb;
    Voice movieVoice;
    // decoded on a worker thread (the ambience mix takes tens of ms): started once ready
    std::future<std::shared_ptr<Pcm>> feAmbRead;
    std::shared_ptr<Pcm> feAmbPcm;   // feAmbRead's loop once decoded, started where the page allows it
    MusicSequence raceMusic{output, openSong};   // the track's <t>_musicN.wav or the custom music
    Voice previewVoice;   // FE LEVELMUSIC "Preview Song"
    Voice engineTestVoice;   // FE OPTIONS SOUND's Engine Volume row
    Voice rainVoice;    // SF_GENERIC_RAINWINDSHIELD
    RaceSfx raceSfx{output};   // the race's generic sounds by id (race_sfx.hpp)
    Voice tunnelVoice;         // the track's SF_<T>_TUNNEL
    struct OneShot
    {
        std::shared_ptr<Pcm> pcm;
    };   // L/R pairs merged into one stereo buffer
    std::vector<OneShot> oneShots;
    struct Playing
    {
        Voice voice;
    };
    std::vector<Playing> playing;
    std::vector<Playing> uiPlaying;   // the menus' sounds: stopAll / stopRaceEvents leave them to play out
    std::vector<std::pair<float, int>> scheduled;   // (seconds left, one-shot index)

    void holdRace();     // the pause menu: every race voice holds
    void resumeRace();

    std::shared_ptr<Pcm> load(const fs::path &path)
    {
        return reader.load(path);
    }

    // a loop gets a click-free seam first (startLoop); nothing without a device or a sound
    bool start(Voice &voice, std::shared_ptr<Pcm> pcm, Playback playback)
    {
        if (playback == Playback::Looped)
        {
            return startLoop(output, voice, std::move(pcm));
        }
        if (!output.isOpen() || !pcm || pcm->samples.empty())
        {
            return false;
        }
        return voice.start(output, std::move(pcm), Playback::Once);
    }
};

Audio::Audio() : impl_(std::make_unique<Impl>())
{
    impl_->reader.warnings = &warnings_;
}
Audio::~Audio()
{
    stopAll();
}

void Audio::init(mix::SpeakerLayout layout)
{
    impl_->output.open(layout);
    ready_ = true;
}

void Audio::stopAll()
{
    auto &impl = *impl_;
    stopRaceEvents();
    impl.trackLoops.stop();
    impl.sceneAmbienceVoice.stop();
    impl.engines.stop();
    impl.movieVoice.stop();
    impl.engineTestVoice.stop();
    impl.rainVoice.stop();
    impl.tunnelVoice.stop();
}

void Audio::stopRaceEvents()
{
    auto &impl = *impl_;
    for (auto &playing : impl.playing)
    {
        playing.voice.stop();
    }
    impl.raceMusic.stop();
    impl.raceMusicFade = 1.f;
    impl.previewVoice.stop();
    impl.raceSfx.stopAll();
    impl.playing.clear();
    impl.scheduled.clear();
}

// Without an audio device (screenshot mode) the emitters still arrive, for the markers.
void Audio::playTrackSounds(TrackSounds sounds)
{
    auto &impl = *impl_;
    stopVoices(impl.playing);
    impl.playing.clear();
    impl.scheduled.clear();
    impl.oneShots.clear();
    impl.reader.cache.clear();
    impl.sceneAmbienceVoice.stop();
    impl.sceneAmbience = std::move(sounds.ambient);
    impl.trackLoops.play(sounds);
    emitters_ = std::move(sounds.emitters);
    oneShotNames_.clear();
    warnings_ = std::move(sounds.warnings);
    for (auto &[name, pcm] : sounds.oneShots)
    {
        impl.oneShots.push_back({pcm});
        oneShotNames_.push_back(name);
    }
    impl.raceMusicModes = MusicModes(sounds.music);
    impl.raceMusic.setPlaylist(sounds.music.playlist(), impl.raceMusicModes);
    impl.raceMusicFade = 1.f;
    impl.raceMusicModes.set(MusicModes::Intro);   // 0xc8e10 at the race's load
}

void Audio::setMusicList(const std::vector<fs::path> &files)
{
    if (!impl_)
    {
        return;
    }
    static std::random_device seeds;
    impl_->customMusicOrder = RandomPickOrder(seeds());
    impl_->raceMusic.setPlaylist(files, impl_->customMusicOrder);
    impl_->raceMusicFade = 1.f;
}

void Audio::preview(const fs::path &file)
{
    if (!ok())
    {
        return;
    }
    auto &impl = *impl_;
    impl.previewVoice.stop();
    auto pcm = impl.load(file);
    impl.reader.cache.erase(pathKey(file));
    if (!impl.start(impl.previewVoice, pcm, Playback::Once))
    {
        return;
    }
    impl.previewVoice.setGains(1.f, 1.f);
    impl.previewVoice.setVolume(musicVol * mix::gainOf(mix::kMusicMb));
    impl.output.setMasterVolume(muted ? 0.f : master);
}

void Audio::loadUi(const std::string &game)
{
    if (!ok() || !impl_->ui.empty())
    {
        return;
    }
    auto &impl = *impl_;
    loadHrtf(game);
    static const char *names[6] = {"ui_button1p", "ui_rollover3", "ui_rollover1", "ui_xition1_LR", "ui_xition2", "ui_xition3_LR"};
    fs::path dir = findChild(fs::path(game) / "audio", "frontend");
    for (const char *name : names)
    {
        std::vector<SpeakerPart> parts;
        for (auto &file : listGameSounds(findChild(dir, name)))
        {
            parts.push_back({impl.load(file), mix::speakerFileGains(lower(file.stem().string()))});
        }
        impl.ui.push_back(parts.empty() ? nullptr : mixSpeakerFiles(parts, false));
    }
}

void Audio::playUi(int id)
{
    if (!ok() || id < 0 || id >= int(impl_->ui.size()) || !impl_->ui[size_t(id)])
    {
        return;
    }
    auto &impl = *impl_;
    dropFinishedVoices(impl.uiPlaying);
    Impl::Playing playing;
    playing.voice.setBus(Bus::Sfx);
    if (!impl.start(playing.voice, impl.ui[size_t(id)], Playback::Once))
    {
        return;
    }
    playing.voice.startRouting(impl.output.routingWithSpeakers({}, mix::kNoPositionSpeakers));   // every front-end sound is NOPOSITION
    playing.voice.setGains(1.f, 1.f);
    playing.voice.setVolume(sfxVol);
    impl.uiPlaying.push_back(std::move(playing));
}

static bool readFinished(const std::future<std::shared_ptr<Pcm>> &read)
{
    return read.valid() && read.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

void Audio::setFrontEnd(const std::string &game, int mode)
{
    if (!ok() || mode == feMode_)
    {
        return;
    }
    auto &impl = *impl_;
    feMode_ = mode;
    impl.frontEndMusic.setPlaylist({});
    impl.feAmb.stop();
    impl.feAmbRead = {};
    impl.feAmbPcm = nullptr;
    if (mode == 0)
    {
        return;
    }
    const fs::path musicRoot = fs::path(game) / "media" / "music";
    if (mode == 1)
    {
        playFrontEndMusic(MusicFolder::read(findChild(musicRoot, "frontend_music")));
    }
    else
    {
        impl.frontEndMusic.setPlaylist(listGameSounds(findChild(musicRoot, "levelload_music")));
    }
    impl.frontEndMusic.start();
    impl.feAmbRead = std::async(std::launch::async, readFrontEndAmbient, game, mode);
}

// the front end's music in the game's modes (entry 0 of 0xded40), from the first segment (0x27764 and the others)
void Audio::playFrontEndMusic(const MusicFolder &folder)
{
    auto &impl = *impl_;
    impl.frontEndMusicModes = MusicModes(folder);
    impl.frontEndMusic.setPlaylist(folder.playlist(), impl.frontEndMusicModes);
}

// stderr: each race song as it starts (the order the music modes pick)
void Audio::logSongChange()
{
    const std::string song = impl_->raceMusic.nowPlaying();
    if (song != loggedSong_)
    {
        loggedSong_ = song;
        std::fprintf(stderr, "music: %s (mode %d)\n", song.empty() ? "-" : song.c_str(), impl_->raceMusicModes.mode());
    }
}

void Audio::playRaceFinale()
{
    if (ok())
    {
        impl_->raceMusicModes.playFinale();
    }
}

void Audio::setRaceMusicFade(float gain)
{
    if (impl_)
    {
        impl_->raceMusicFade = gain;
    }
}

void Audio::startRaceMusic()
{
    if (ok())
    {
        impl_->raceMusicModes.set(MusicModes::OnStart);
    }
}

void Audio::restartRaceMusic(int mode)
{
    if (!ok())
    {
        return;
    }
    impl_->raceMusic.stop();
    impl_->raceMusicFade = 1.f;
    impl_->raceMusicModes.startOver(mode);
}

void Audio::playSecondLapMusic()
{
    if (ok())
    {
        impl_->raceMusicModes.set(MusicModes::SecondLap);
    }
}

void Audio::raiseRaceMusic()
{
    if (ok())
    {
        impl_->raceMusicModes.stepUp();
    }
}

void Audio::lowerRaceMusic()
{
    if (ok())
    {
        impl_->raceMusicModes.stepDown();
    }
}

void Audio::updateFrontEnd()
{
    if (!ok() || !feMode_)
    {
        return;
    }
    auto &impl = *impl_;
    impl.output.setMasterVolume(muted ? 0.f : master);
    startFrontEndAmbience();
    if (impl.feAmb.active())
    {
        // the menus' ui_amb3 through sound_play_ui (the SFX option); the loading screen's ui_levelload at the Music
        // Volume (0xca610: 0xdee90)
        impl.feAmb.setVolume(feMode_ == 1 ? ambientVol * sfxVol : musicVol);
    }
    impl.frontEndMusic.update();
    const bool engineTest = impl.engineTestVoice.active();
    impl.frontEndMusic.setVolume(music && !engineTest ? musicVol * mix::gainOf(mix::kMusicMb) : 0.f);
    if (engineTest)
    {
        impl.engineTestVoice.setVolume(engineVol);
    }
    dropFinishedVoices(impl.uiPlaying);
}

void Audio::playSceneAmbience()
{
    auto &impl = *impl_;
    impl.sceneAmbienceVoice.setBus(Bus::Sfx);
    if (ok() && impl.start(impl.sceneAmbienceVoice, impl.sceneAmbience, Playback::Looped))
    {
        impl.sceneAmbienceVoice.setGains(1.f, 1.f);
        impl.sceneAmbienceVoice.setVolume(ambient ? ambientVol * sfxVol * mix::gainOf(mix::kSceneAmbienceMb) : 0.f);
    }
}

void Audio::stopSceneAmbience()
{
    impl_->sceneAmbienceVoice.stop();
}

void Audio::enterFrontEndPage(bool ambience)
{
    feAmbWanted_ = ambience;
    if (ok() && feMode_ == 1)
    {
        impl_->feAmb.stop();
    }
}

// the loop once decoded: the menus' when the page's enter asked for it, the loading screen's always
void Audio::startFrontEndAmbience()
{
    auto &impl = *impl_;
    if (readFinished(impl.feAmbRead))
    {
        impl.feAmbPcm = impl.feAmbRead.get();
    }
    const bool wanted = feMode_ != 1 || feAmbWanted_;
    impl.feAmb.setBus(Bus::Sfx);
    if (wanted && !impl.feAmb.active() && impl.start(impl.feAmb, impl.feAmbPcm, Playback::Looped))
    {
        impl.feAmb.setGains(1.f, 1.f);
    }
}

// the race's voices hold (ambience, emitters, engine, its one-shots, music); new voices (the UI sounds) still play
void Audio::playMovie(std::shared_ptr<Pcm> pcm)
{
    auto &impl = *impl_;
    impl.movieVoice.stop();
    if (!ok() || !impl.start(impl.movieVoice, std::move(pcm), Playback::Once))
    {
        return;
    }
    impl.movieVoice.setGains(1.f, 1.f);
    impl.movieVoice.setVolume(1.f);
}

void Audio::stopMovie()
{
    impl_->movieVoice.stop();
}

double Audio::movieSeconds() const
{
    return impl_->movieVoice.secondsPlayed();
}

void Audio::holdRaceSounds()
{
    if (impl_->output.isOpen())
    {
        impl_->holdRace();
    }
}

void Audio::resumeRaceSounds()
{
    if (impl_->output.isOpen())
    {
        impl_->resumeRace();
    }
}

void Audio::suspendOutput()
{
    impl_->output.suspend();
}

void Audio::resumeOutput()
{
    impl_->output.resume();
}

void Audio::Impl::holdRace()
{
    trackLoops.hold();
    engines.hold();
    holdVoices(playing);
    rainVoice.pause();
    tunnelVoice.pause();
    raceSfx.pause();
    raceMusic.pause();
}

void Audio::Impl::resumeRace()
{
    trackLoops.resume();
    engines.resume();
    resumeVoices(playing);
    rainVoice.resume();
    tunnelVoice.resume();
    raceSfx.resume();
    raceMusic.resume();
}

RaceSfx &Audio::raceSfx()
{
    return impl_->raceSfx;
}

// the 3D voices' HRTF tables, once: audio\hrtf.bin (tools\assets\extract_hrtf.py), else default.xbe
void Audio::loadHrtf(const std::string &game)
{
    const hrtf::TableFiles files{findGamePath(fs::path(game) / "audio" / "hrtf.bin"), fs::path(game) / "default.xbe"};
    try
    {
        impl_->output.loadHrtf(files);
    }
    catch (const std::runtime_error &e)
    {
        warnings_.push_back(std::string(e.what()) + ": 3D sounds play without the HRTF");
    }
    loadKemar();
}

// MIT KEMAR's tables when tools\assets\convert_kemar.py wrote them, so Argon Options can switch to them at any time
void Audio::loadKemar()
{
    if (impl_->output.hasKemar() || (!fs::exists(kemarFile_) && hrtfSet_ == hrtf::Set::Xbox))
    {
        return;
    }
    try
    {
        impl_->output.loadKemar(kemarFile_);
    }
    catch (const std::runtime_error &e)
    {
        warnings_.push_back(std::string(e.what()) + ": 3D sounds play through the Xbox's HRTF");
    }
}

void Audio::setKemarFile(const fs::path &file)
{
    kemarFile_ = file;
}

void Audio::setHrtfSet(hrtf::Set set)
{
    hrtfSet_ = set;
    impl_->output.setHrtfSet(set);
}

void Audio::setKemarTaps(int taps)
{
    impl_->output.setKemarTaps(taps);
}

// audio\filters.txt, read once
void Audio::loadListenerFilters(const std::string &game)
{
    loadHrtf(game);
    if (WorldEffects *world = impl_->output.worldEffects())
    {
        world->setFilters(parseListenerFilters(readTextFile(fs::path(game) / "audio" / "filters.txt")));
    }
}

void Audio::selectListenerFilter(WorldEffects::Filter filter)
{
    if (WorldEffects *world = impl_->output.worldEffects())
    {
        world->select(filter);
    }
}

void Audio::playEngineTest(const std::string &game)
{
    if (!ok() || impl_->engineTestVoice.active())
    {
        return;
    }
    auto &impl = *impl_;
    const auto files = listGameSounds(findChild(findChild(fs::path(game) / "audio", "frontend"), "enginetest"));
    impl.engineTestVoice.setBus(Bus::Sfx);
    if (files.empty() || !impl.start(impl.engineTestVoice, impl.load(files.front()), Playback::Looped))
    {
        return;
    }
    impl.engineTestVoice.setGains(1.f, 1.f);
    impl.engineTestVoice.setVolume(engineVol);
}

void Audio::stopEngineTest()
{
    if (impl_)
    {
        impl_->engineTestVoice.stop();
    }
}

void Audio::playRain(const std::string &game)
{
    if (!ok() || impl_->rainVoice.active())
    {
        return;
    }
    auto &impl = *impl_;
    impl.rainVoice.setBus(Bus::World);
    if (!impl.start(impl.rainVoice, readGenericSound(game, "rain_windshield"), Playback::Looped))
    {
        return;
    }
    impl.rainVoice.setGains(1.f, 1.f);
    impl.rainVoice.setVolume(sfxVol);
}

void Audio::stopRain()
{
    if (impl_)
    {
        impl_->rainVoice.stop();
    }
}

void Audio::stopPreview()
{
    if (impl_)
    {
        impl_->previewVoice.stop();
    }
}

void Audio::loadEngine(const std::string &game, const std::string &pilot)
{
    if (!ok())
    {
        return;
    }
    playEngineSounds(readEngineSounds(game, pilot));
}

void Audio::playEngineSounds(EngineSounds sounds)
{
    if (!ok())
    {
        return;
    }
    for (auto &warning : sounds.warnings)
    {
        warnings_.push_back(warning);
    }
    impl_->engines.playOwn(sounds);
}

void Audio::playOtherEngines(std::vector<EngineSounds> engines)
{
    if (!ok())
    {
        return;
    }
    impl_->engines.playOthers(engines);
}

void Audio::update(float dt, const glm::vec3 &camPos, const glm::vec3 &forward, const glm::vec3 &up)
{
    if (!ok())
    {
        return;
    }
    auto &impl = *impl_;
    impl.output.setMasterVolume(muted ? 0.f : master);
    const glm::vec3 right = glm::normalize(glm::cross(forward, up));
    const spatial::Listener listener{camPos, right, listenerVelocity, forward, up};
    impl.raceSfx.setListener(listener);
    impl.raceSfx.update(dt, sfxVol);
    const TrackLoops::Levels loops{emitterSounds ? emitterVol * sfxVol : 0.f};
    impl.trackLoops.update(listener, loops, dt);
    const float engineGain = engine ? engineVol : 0.f;
    impl.engines.driveOwn(throttle, engineGain);
    impl.engines.driveOthers(otherEngines, listener, EngineVoices::OthersLevel{engineGain, enginesOnListener});
    // the music playlist, back to back (switched off: it resumes on the same song)
    if (music && impl.raceMusic.hasSongs())
    {
        impl.raceMusic.start();
        impl.raceMusic.update();
        impl.raceMusic.setVolume(musicVol * mix::gainOf(mix::kMusicMb) * impl.raceMusicFade);
        logSongChange();
    }
    else
    {
        impl.raceMusic.stop();
        impl.raceMusicFade = 1.f;
    }
    // one-shots and the countdown schedule
    for (auto &pending : impl.scheduled)
    {
        pending.first -= dt;
    }
    for (auto it = impl.scheduled.begin(); it != impl.scheduled.end();)
    {
        if (it->first <= 0)
        {
            playOneShot(it->second);
            it = impl.scheduled.erase(it);
        }
        else
        {
            ++it;
        }
    }
    dropFinishedVoices(impl.playing);
}

void Audio::playOneShot(int index)
{
    if (!ok() || index < 0 || index >= int(impl_->oneShots.size()))
    {
        return;
    }
    playSfx(impl_->oneShots[index].pcm);
}

// a one-shot at the sound-effects volume, not positional
void Audio::playSfx(const std::shared_ptr<Pcm> &pcm)
{
    auto &impl = *impl_;
    Impl::Playing playing;
    playing.voice.setBus(Bus::Sfx);
    if (!pcm || !impl.start(playing.voice, pcm, Playback::Once))
    {
        return;
    }
    playing.voice.setGains(1.f, 1.f);
    playing.voice.setVolume(sfxVol);
    impl.playing.push_back(std::move(playing));
}

void Audio::playOneShotNamed(const char *name)
{
    if (ok())
    {
        playOneShot(oneShotIndex(name));
    }
}

int Audio::oneShotIndex(const char *name) const
{
    for (size_t i = 0; i < oneShotNames_.size(); ++i)
    {
        if (lower(oneShotNames_[i]) == name)
        {
            return int(i);
        }
    }
    return -1;
}

// 0xc4220: under a roof the track's SF_<T>_TUNNEL starts whenever it is not playing (so it repeats there); out of
// it, it stops (0xc4af0). On the listener (no owner), at the SFX volume.
void Audio::setTunnel(bool inside)
{
    auto &impl = *impl_;
    if (!inside)
    {
        impl.tunnelVoice.stop();
        return;
    }
    const int index = oneShotIndex("tunnel");
    if (!ok() || index < 0 || !impl.tunnelVoice.finished())
    {
        return;
    }
    impl.tunnelVoice.setBus(Bus::World);
    if (impl.start(impl.tunnelVoice, impl.oneShots[size_t(index)].pcm, Playback::Once))
    {
        impl.tunnelVoice.setGains(1.f, 1.f);
        impl.tunnelVoice.setVolume(sfxVol);
    }
}

void Audio::playCountdown()
{
    if (!ok())
    {
        return;
    }
    const char *seq[] = {"three", "two", "one", "go"};
    for (int k = 0; k < 4; ++k)
    {
        for (size_t i = 0; i < oneShotNames_.size(); ++i)
        {
            if (lower(oneShotNames_[i]) == seq[k])
            {
                impl_->scheduled.push_back({float(k), int(i)});
            }
        }
    }
}

std::string Audio::nowPlaying() const
{
    return impl_ ? impl_->raceMusic.nowPlaying() : std::string();
}

int Audio::engineLayers() const
{
    return impl_ ? impl_->engines.ownLayers() : 0;
}

} // namespace argon
