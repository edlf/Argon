// Argon: the race's generic sounds and audio\f_settings.txt, see generic_sounds.hpp.
#include "audio/generic_sounds.hpp"

#include <cstdlib>
#include <fstream>
#include <sstream>

#include "audio/audio.hpp"
#include "audio/ship_sound_list.hpp"
#include "files/game_path.hpp"

namespace argon
{
namespace
{
namespace fs = std::filesystem;

std::string readText(const fs::path &path)
{
    std::ifstream in(findGamePath(path));
    std::ostringstream text;
    text << in.rdbuf();
    return text.str();
}

fs::path genericFolder(const std::string &game)
{
    return fs::path(game) / "audio" / "generic";
}

// generic.txt's folder for each id (empty when the list does not name the sound)
std::array<std::string, kGenericSoundCount> folderOfEachSound(const std::string &game)
{
    std::array<std::string, kGenericSoundCount> folders;
    std::istringstream list(readText(genericFolder(game) / "generic.txt"));
    std::string name, folder;
    while (list >> name >> folder)
    {
        for (size_t id = 1; id < kGenericSoundCount; ++id)
        {
            if (name == std::string("SF_GENERIC_") + kGenericSoundNames[id])
            {
                folders[id] = folder;
            }
        }
    }
    return folders;
}

// TYPE <kind>: WEAPON also sets MINDIST 600 / MAXDIST 700 (the parser at 0xc7283); the other kinds change nothing
// played here
void readType(std::istringstream &in, SoundOptions &options)
{
    std::string kind;
    in >> kind;
    if (kind == "WEAPON")
    {
        options.minDist = 600.f;
        options.maxDist = 700.f;
    }
}

// the parser's speaker keywords (0xc6fbc..0xc7060), also ROUTETO's targets: the 2D speaker voices the sound plays on
void readSpeakerFlag(const std::string &word, mix::SpeakerFlags &speakers)
{
    if (word == "CENTRE")
    {
        speakers.centre = true;
    }
    else if (word == "LFE")
    {
        speakers.lfe = true;
    }
    else if (word == "LS")
    {
        speakers.ls = true;
    }
    else if (word == "RS")
    {
        speakers.rs = true;
    }
    else if (word == "RIGHT")
    {
        speakers.right = true;
    }
    else if (word == "LEFT")
    {
        speakers.left = true;
    }
}

SoundClip readClip(const fs::path &folder)
{
    SoundClip clip;
    clip.options = parseSoundOptions(readText(folder / (folder.filename().string() + ".txt")));
    clip.pcm = readSoundFolder(findGamePath(folder));
    if (clip.pcm && clip.options.loop)
    {
        makeLoopable(*clip.pcm);
    }
    return clip;
}
}  // namespace

SoundOptions parseSoundOptions(const std::string &text)
{
    SoundOptions options;
    std::istringstream in(text);
    std::string word;
    while (in >> word)
    {
        if (word == "LOOPS")
        {
            int loops = 0;
            in >> loops;
            options.loop = loops != 0;
            sound::scanLoopsInto(loops, options.volume);
        }
        else if (word == "MINVOL")
        {
            int minMb = 0;
            in >> minMb;
            sound::scanMinVolumeInto(minMb, options.volume);
        }
        else if (word == "MAXVOL")
        {
            in >> options.volume.maxMb;
        }
        else if (word == "MINDIST")
        {
            in >> options.minDist;
        }
        else if (word == "MAXDIST")
        {
            in >> options.maxDist;
        }
        else if (word == "NOPOSITION")
        {
            options.positional = false;
            options.speakers.centre = options.speakers.right = options.speakers.left = true;   // 0xc7346: +0x91 / +0x95 / +0x96
        }
        else if (word == "TYPE")
        {
            readType(in, options);
        }
        else
        {
            readSpeakerFlag(word, options.speakers);
        }
    }
    return options;
}

GenericSoundBank readGenericSounds(const std::string &game)
{
    GenericSoundBank bank;
    const auto folders = folderOfEachSound(game);
    for (size_t id = 1; id < kGenericSoundCount; ++id)
    {
        if (!folders[id].empty())
        {
            bank[id] = readClip(genericFolder(game) / folders[id]);
        }
    }
    return bank;
}

ShipSounds readShipSounds(const std::string &game, const std::string &pilot)
{
    ShipSounds sounds;
    const fs::path dir = fs::path(game) / "audio" / "ships" / pilot;
    std::istringstream list(readText(dir / (pilot + ".txt")));
    const std::map<std::string, std::string> folders = readShipSoundList(list);
    for (size_t id = 0; id < kShipSoundCount; ++id)
    {
        const auto it = folders.find(std::string("SF_SHIP_") + kShipSoundNames[id]);
        if (it != folders.end())
        {
            sounds[id] = readClip(dir / it->second);
        }
    }
    return sounds;
}

sound::ImpactSettings readImpactSettings(const std::string &game)
{
    return sound::parseImpactSettings(readText(fs::path(game) / "audio" / "f_settings.txt"));
}

}  // namespace argon
