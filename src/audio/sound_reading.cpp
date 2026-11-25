// Argon: reading the game's sounds from disk: the track's list, ambience and emitters, the engines, sound folders
// (docs\audio\README.md). Any thread; the voices that play them are audio.cpp's.
#include "audio/sound_reader.hpp"
#include "audio.hpp"
#include "files/game_path.hpp"

#include "audio/audio_output.hpp"
#include "audio/game_sound_files.hpp"
#include "audio/mix_paths.hpp"
#include "audio/music_sequence.hpp"
#include "audio/race_sfx.hpp"
#include "audio/song_files.hpp"
#include "audio/song_sources.hpp"
#include "audio/world_effects.hpp"
#include "audio/voice.hpp"

#include <algorithm>
#include <stdexcept>
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

namespace argon
{

static fs::path playableSoundFile(const fs::path &wav)
{
    for (const fs::path &converted : convertedSoundCandidates(wav))
    {
        const fs::path found = findGamePath(converted);
        std::error_code ec;
        if (fs::is_regular_file(found, ec))
        {
            return found;
        }
    }
    return wav;
}

Pcm loadGameSound(const fs::path &wav)
{
    return loadSound(playableSoundFile(wav));
}

std::string readTextFile(const fs::path &path)
{
    std::ifstream in(findGamePath(path));
    std::ostringstream text;
    text << in.rdbuf();
    return text.str();
}

// case-insensitive child lookup (the game's file names are mixed case)
fs::path findChild(const fs::path &dir, const std::string &name)
{
    std::error_code ec;
    if (!fs::is_directory(dir, ec))
    {
        return {};
    }
    std::string want = lower(name);
    for (auto &entry : fs::directory_iterator(dir, ec))
    {
        if (lower(entry.path().filename().string()) == want)
        {
            return entry.path();
        }
    }
    return {};
}

// the sound `wavName` names in `dir` (any case), also when only its converted copy is there; empty when there is none
static fs::path findSoundIn(const fs::path &dir, const std::string &wavName)
{
    const std::string wanted = lower(wavName);
    for (const fs::path &sound : listGameSounds(dir))
    {
        if (lower(sound.filename().string()) == wanted)
        {
            return sound;
        }
    }
    return {};
}

static std::vector<fs::path> filesWithExt(const fs::path &dir, const std::string &ext)
{
    std::vector<fs::path> out;
    std::error_code ec;
    if (!fs::is_directory(dir, ec))
    {
        return out;
    }
    for (auto &entry : fs::directory_iterator(dir, ec))
    {
        if (entry.is_regular_file() && lower(entry.path().extension().string()) == ext)
        {
            out.push_back(entry.path());
        }
    }
    std::sort(out.begin(), out.end());
    return out;
}

// Make a buffer loop without a click: crossfade its last N frames into its first N, then loop
// over [N, end). The sample before the wrap now flows into frame N, whatever the recording's
// seam looks like (some shipped loops, e.g. Silver's ocean, jump ~10x a normal step).
void makeLoopable(Pcm &pcm)
{
    if (pcm.loopReady)
    {
        return;
    }
    pcm.loopReady = true;
    size_t frames = pcm.samples.size() / pcm.channels;
    size_t fadeFrames = std::min<size_t>(size_t(pcm.rate) / 80, frames / 4);   // ~12 ms
    if (fadeFrames < 8)
    {
        return;
    }
    for (size_t i = 0; i < fadeFrames; ++i)
    {
        float w = float(i + 1) / float(fadeFrames + 1);
        for (int c = 0; c < pcm.channels; ++c)
        {
            size_t tail = (frames - fadeFrames + i) * pcm.channels + c, head = i * pcm.channels + c;
            pcm.samples[tail] = int16_t(std::clamp(pcm.samples[tail] * (1.f - w) + pcm.samples[head] * w, -32768.f, 32767.f));
        }
    }
    pcm.loopBegin = fadeFrames;
}

namespace
{
// one file's frame on the buffer's channels: stereo by its downmix (a left / right file keeps its sides), the six
// speaker mixbins each from the file's side of it (the centre and the LFE from both)
void addFrame(const Pcm &file, size_t frame, const mix::SpeakerGains &gains, int channels, float *to)
{
    const float left = file.samples[frame * size_t(file.channels)];
    const float right = file.channels > 1 ? file.samples[frame * size_t(file.channels) + 1] : left;
    if (channels == 2)
    {
        const glm::vec2 stereo = mix::stereoDownmix(gains);
        to[0] += left * stereo.x;
        to[1] += right * stereo.y;
        return;
    }
    const float both = (left + right) * 0.5f;
    const std::array<float, mix::kSpeakerCount> sideOf = {left, right, both, both, left, right};
    for (size_t speaker = 0; speaker < mix::kSpeakerCount; ++speaker)
    {
        to[speaker] += sideOf[speaker] * gains[speaker];
    }
}
}  // namespace

// Mix mono/stereo files into one buffer: a single file as stereo (its 3D voice), several as the six speaker mixbins
// they play on (the 2D speaker voices; WorldEffects downmixes them for stereo). Playing L/R files as separate voices
// lets them start a few ms apart, which smears (phases) near-identical channels such as the announcer; one voice can't
// drift.
std::shared_ptr<Pcm> mixSpeakerFiles(const std::vector<SpeakerPart> &parts, bool normalise)
{
    int rate = 0;
    size_t frames = 0, play = 0;
    for (auto &part : parts)
    {
        if (part.first)
        {
            rate = rate ? rate : part.first->rate;
            frames = std::max(frames, part.first->samples.size() / part.first->channels);
            play = std::max(play, part.first->playFrames);
        }
    }
    if (!rate || !frames)
    {
        return nullptr;
    }
    const bool speakerFiles = parts.size() > 1;
    const int channels = speakerFiles ? int(mix::kSpeakerCount) : 2;
    std::vector<float> acc(frames * size_t(channels), 0.f);
    for (auto &part : parts)
    {
        if (!part.first || part.first->rate != rate)
        {
            continue;
        }
        const Pcm &pcm = *part.first;
        size_t partFrames = pcm.samples.size() / pcm.channels;
        for (size_t i = 0; i < partFrames; ++i)
        {
            addFrame(pcm, i, part.second, channels, acc.data() + i * size_t(channels));
        }
    }
    float peak = 1.f;
    for (float v : acc)
    {
        peak = std::max(peak, std::abs(v));
    }
    float gain = normalise && peak > 32767.f ? 32767.f / peak : 1.f;
    auto out = std::make_shared<Pcm>();
    out->channels = channels;
    out->rate = rate;
    out->playFrames = play;
    out->speakerFiles = speakerFiles;
    out->storedScale = gain;
    for (auto &part : parts)
    {
        if (part.first && part.first->sampleLoopEnd > out->sampleLoopEnd)
        {
            out->sampleLoopBegin = part.first->sampleLoopBegin;
            out->sampleLoopEnd = std::min(part.first->sampleLoopEnd, frames);
        }
    }
    out->samples.resize(acc.size());
    for (size_t i = 0; i < acc.size(); ++i)
    {
        out->samples[i] = int16_t(std::clamp(acc[i] * gain, -32768.f, 32767.f));
    }
    return out;
}

static std::vector<std::string> tokensOf(const std::string &line)
{
    std::istringstream is(line);
    std::vector<std::string> fields;
    std::string word;
    while (is >> word)
    {
        fields.push_back(word);
    }
    return fields;
}

std::shared_ptr<Pcm> SoundReader::load(const fs::path &file)
{
    const std::string key = pathKey(file);
    auto cached = cache.find(key);
    if (cached != cache.end())
    {
        return cached->second;
    }
    std::shared_ptr<Pcm> pcm;
    try
    {
        pcm = std::make_shared<Pcm>(loadGameSound(file));
    }
    catch (const std::runtime_error &e)
    {
        if (warnings)
        {
            warnings->push_back(e.what());
        }
    }
    cache[key] = pcm;
    return pcm;
}

std::shared_ptr<Pcm> SoundReader::loadLoop(const fs::path &file)
{
    std::shared_ptr<Pcm> pcm = load(file);
    if (pcm)
    {
        makeLoopable(*pcm);
    }
    return pcm;
}

// one song opened to be decoded as it plays (FLAC / Ogg / MP3; a WAV decoded whole), not cached, any thread
std::shared_ptr<SongFeed> openSong(const fs::path &song)
{
    const fs::path file = playableSoundFile(song);
    std::ifstream f(findGamePath(file), std::ios::binary);
    if (!f)
    {
        return nullptr;
    }
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    std::unique_ptr<SongFrames> frames = compressedSongFrames(file, std::move(bytes));
    if (!frames)
    {
        std::shared_ptr<Pcm> pcm;
        try
        {
            pcm = std::make_shared<Pcm>(loadSound(file));
        }
        catch (const std::runtime_error &)
        {
            return nullptr;
        }
        if (pcm->samples.empty())
        {
            return nullptr;
        }
        frames = std::make_unique<PcmFrames>(std::move(pcm));
    }
    auto feed = std::make_shared<SongFeed>(std::move(frames));
    return feed->ok() ? feed : nullptr;
}

// the files of a track sound folder: its *_L / *_R pair (sorted: L first), else its first file
static std::vector<fs::path> soundFolderFiles(const fs::path &tdir, const std::string &folder)
{
    auto wavs = listGameSounds(findChild(tdir, folder));
    std::vector<fs::path> lr;
    for (auto &wav : wavs)
    {
        std::string name = lower(wav.stem().string());
        if (name.size() > 2 && (name.substr(name.size() - 2) == "_l" || name.substr(name.size() - 2) == "_r"))
        {
            lr.push_back(wav);
        }
    }
    return lr.size() == 2 ? lr : (wavs.empty() ? wavs : std::vector<fs::path>{wavs.front()});
}

// the 5.1 ambience (Amb_N.L / R / C / Ls / Rs / LFE) as one loop on its speaker mixbins, normalised so it can't clip
static std::shared_ptr<Pcm> readAmbient(const fs::path &tdir, SoundReader &reader)
{
    std::vector<SpeakerPart> parts;
    for (auto &file : listGameSounds(findChild(tdir, "cut_ambient")))
    {
        parts.push_back({reader.load(file), mix::speakerFileGains(lower(file.stem().string()))});
    }
    std::shared_ptr<Pcm> mixed = mixSpeakerFiles(parts, true);
    if (mixed)
    {
        makeLoopable(*mixed);
    }
    return mixed;
}

// a sound without TRIGGER points (countdown, tunnel, thunder, ...): its L/R pair as one one-shot, on its
// speaker mixbins (mix::speakerFileGains)
static std::shared_ptr<Pcm> readOneShot(const fs::path &tdir, const std::string &folder, SoundReader &reader)
{
    std::vector<SpeakerPart> parts;
    for (const fs::path &file : soundFolderFiles(tdir, folder))
    {
        parts.push_back({reader.load(file), mix::speakerFileGains(lower(file.stem().string()))});
    }
    return mixSpeakerFiles(parts, true);
}

// <track>.txt: "SF_NAME <folder>" entries, positional ones with TRIGGER points (X / Y / Z / MINDIST / MAXDIST); a
// sound without TRIGGER points is a one-shot (read only when decoding)
class SoundListReader
{
  public:
    SoundListReader(const fs::path &trackDir, SoundReader *reader, TrackSounds &out) : trackDir_(trackDir), reader_(reader), out_(out)
    {
    }
    void read(std::istream &list)
    {
        std::string line;
        while (std::getline(list, line))
        {
            readTokens(tokensOf(line));
        }
        finishSound();
    }

  private:
    void readTokens(const std::vector<std::string> &fields)
    {
        if (fields.empty())
        {
            return;
        }
        const bool valued = fields.size() >= 2;
        if (fields[0].rfind("SF_", 0) == 0 && valued)
        {
            finishSound();
            sound_ = fields[0];
            folder_ = fields[1];
            hadTrigger_ = false;
            inTrigger_ = false;
        }
        else if (fields[0] == "TRIGGER")
        {
            inTrigger_ = true;
            hadTrigger_ = true;
            point_ = SoundEmitter{sound_, folder_, glm::vec3(0), 0.f, 0.f};
        }
        else if (inTrigger_ && valued && (fields[0] == "X" || fields[0] == "Y" || fields[0] == "Z"))
        {
            point_.pos[fields[0][0] - 'X'] = float(std::atof(fields[1].c_str()));
        }
        else if (inTrigger_ && valued && fields[0] == "MINDIST")
        {
            point_.minDist = float(std::atof(fields[1].c_str()));
        }
        else if (inTrigger_ && valued && fields[0] == "MAXDIST")
        {
            point_.maxDist = float(std::atof(fields[1].c_str()));
            out_.emitters.push_back(point_);   // MAXDIST closes a trigger point
        }
        else if (fields[0] == "TRIGGEREND")
        {
            inTrigger_ = false;
        }
    }

    void finishSound()
    {
        if (sound_.empty())
        {
            return;
        }
        if (!hadTrigger_ && reader_)
        {
            if (auto pcm = readOneShot(trackDir_, folder_, *reader_))
            {
                out_.oneShots.push_back({folder_, pcm});
            }
        }
        sound_.clear();
    }

    const fs::path &trackDir_;
    SoundReader *reader_;
    TrackSounds &out_;
    std::string sound_, folder_;
    SoundEmitter point_{};
    bool inTrigger_ = false, hadTrigger_ = false;
};


// one looping voice per emitter sound (the first file of the sound: mono), shared by its TRIGGER points
static void readEmitterLoops(const fs::path &tdir, SoundReader *reader, TrackSounds &out)
{
    for (size_t i = 0; i < out.emitters.size(); ++i)
    {
        TrackSounds::EmitterLoop *group = nullptr;
        for (auto &loop : out.emitterLoops)
        {
            if (out.emitters[loop.points.front()].name == out.emitters[i].name)
            {
                group = &loop;
            }
        }
        if (!group)
        {
            out.emitterLoops.push_back({});
            group = &out.emitterLoops.back();
            auto files = reader ? soundFolderFiles(tdir, out.emitters[i].file) : std::vector<fs::path>{};
            if (!files.empty())
            {
                group->loop = reader->loadLoop(files.front());
            }
        }
        group->points.push_back(i);
    }
}

TrackSounds readTrackSounds(const std::string &game, const std::string &trackAudio, const std::string &musicTrack, bool decode)
{
    TrackSounds out;
    SoundReader reader;
    reader.warnings = &out.warnings;
    SoundReader *decoder = decode ? &reader : nullptr;
    fs::path tdir = fs::path(game) / "audio" / "tracks" / trackAudio;
    if (decoder)
    {
        out.ambient = readAmbient(tdir, reader);
    }
    std::ifstream list(findChild(tdir, trackAudio + ".txt"));
    SoundListReader(tdir, decoder, out).read(list);
    readEmitterLoops(tdir, decoder, out);
    out.music = MusicFolder::read(findChild(fs::path(game) / "media" / "music", musicTrack + "_music"));
    return out;
}

// audio\frontend ui_amb3 (the menus) / ui_levelload (the loading screen) as one stereo loop, read on any thread
std::shared_ptr<Pcm> readFrontEndAmbient(const std::string &game, int mode)
{
    std::vector<std::string> warnings;
    SoundReader reader;
    reader.warnings = &warnings;
    std::vector<SpeakerPart> parts;
    for (auto &file : listGameSounds(findChild(findChild(fs::path(game) / "audio", "frontend"), mode == 1 ? "ui_amb3" : "ui_levelload")))
    {
        parts.push_back({reader.load(file), mix::speakerFileGains(lower(file.stem().string()))});
    }
    std::shared_ptr<Pcm> mixed = mixSpeakerFiles(parts, true);
    if (mixed)
    {
        makeLoopable(*mixed);
    }
    return mixed;
}

// the game (0xc5700 / 0xc7450) loads engine\ for every craft and h_engine\ (incl. the water loop) for the
// player's craft only; both sets play together. <p>.ini: "Loop", the file, then Vol_i / Pitch_i curve points.
static void readEngineSet(const fs::path &dir, SoundReader &reader, EngineSounds &out)
{
    auto inis = filesWithExt(dir, ".ini");
    if (inis.empty())
    {
        return;
    }
    std::vector<std::string> files;
    size_t first = out.layers.size();
    std::ifstream f(inis.front());
    std::string line;
    bool expectFile = false;
    while (std::getline(f, line))
    {
        auto tokens = tokensOf(line);
        if (tokens.empty())
        {
            continue;
        }
        if (lower(tokens[0]) == "loop")
        {
            out.layers.push_back({});
            files.emplace_back();
            expectFile = true;
            continue;
        }
        if (out.layers.size() == first)
        {
            continue;
        }
        auto &layer = out.layers.back();
        if (expectFile)
        {
            files.back() = line.substr(line.find_first_not_of(" \t"));
            expectFile = false;
            continue;
        }
        if (tokens.size() >= 3 && tokens[0].rfind("Vol_", 0) == 0)
        {
            layer.vol.push_back({float(std::atof(tokens[1].c_str())), float(std::atof(tokens[2].c_str()))});
        }
        if (tokens.size() >= 3 && tokens[0].rfind("Pitch_", 0) == 0)
        {
            layer.pitch.push_back({float(std::atof(tokens[1].c_str())), float(std::atof(tokens[2].c_str()))});
        }
    }
    for (size_t k = 0; k < files.size(); ++k)
    {
        std::string &file = files[k];
        while (!file.empty() && (file.back() == '\r' || file.back() == ' '))
        {
            file.pop_back();
        }
        fs::path soundPath = findSoundIn(dir, file);
        if (soundPath.empty())
        {
            out.warnings.push_back("engine layer missing: " + file);
            continue;
        }
        out.layers[first + k].loop = reader.loadLoop(soundPath);
    }
}

EngineSounds readEngineSounds(const std::string &game, const std::string &pilot)
{
    EngineSounds out;
    SoundReader reader;
    reader.warnings = &out.warnings;
    fs::path ship = findChild(fs::path(game) / "audio" / "ships", pilot);
    for (const char *sub : {"engine", "h_engine"})
    {
        readEngineSet(findChild(ship, sub), reader, out);
    }
    return out;
}

EngineSounds readOtherEngineSounds(const std::string &game, const std::string &pilot)
{
    EngineSounds out;
    SoundReader reader;
    reader.warnings = &out.warnings;
    readEngineSet(findChild(findChild(fs::path(game) / "audio" / "ships", pilot), "engine"), reader, out);
    return out;
}

// a sound folder's files on their speaker mixbins by their suffix (mix::speakerFileGains)
std::shared_ptr<Pcm> readSoundFolder(const fs::path &dir)
{
    std::vector<std::string> warnings;
    SoundReader reader;
    reader.warnings = &warnings;
    std::vector<SpeakerPart> parts;
    for (auto &file : listGameSounds(dir))
    {
        parts.push_back({reader.load(file), mix::speakerFileGains(lower(file.stem().string()))});
    }
    return mixSpeakerFiles(parts, true);
}

std::shared_ptr<Pcm> readGenericSound(const std::string &game, const std::string &folder)
{
    return readSoundFolder(findChild(findChild(fs::path(game) / "audio", "generic"), folder));
}

} // namespace argon
