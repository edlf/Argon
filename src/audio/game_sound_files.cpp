// Argon: the game's sound files by their .wav names, see game_sound_files.hpp.
#include "audio/game_sound_files.hpp"

#include <algorithm>
#include <cctype>
#include <map>
#include <string>
#include <system_error>

namespace argon
{
namespace
{
namespace fs = std::filesystem;

const char *const kWavExtension = ".wav";
const char *const kConvertedExtensions[] = {".flac", ".ogg"};

std::string lowerUtf8(const fs::path &path)
{
    const std::u8string utf8 = path.u8string();
    std::string lower(utf8.begin(), utf8.end());
    for (auto &character : lower)
    {
        character = char(std::tolower(static_cast<unsigned char>(character)));
    }
    return lower;
}

bool isConvertedSound(const fs::path &file)
{
    const std::string extension = lowerUtf8(file.extension());
    return std::find(std::begin(kConvertedExtensions), std::end(kConvertedExtensions), extension) != std::end(kConvertedExtensions);
}

fs::path withWavExtension(fs::path file)
{
    return file.replace_extension(kWavExtension);
}
}  // namespace

std::vector<fs::path> convertedSoundCandidates(const fs::path &wav)
{
    std::vector<fs::path> candidates;
    if (lowerUtf8(wav.extension()) != kWavExtension)
    {
        return candidates;
    }
    for (const char *extension : kConvertedExtensions)
    {
        candidates.push_back(fs::path(wav).replace_extension(extension));
    }
    return candidates;
}

std::vector<fs::path> gameSoundNames(const std::vector<fs::path> &files)
{
    std::map<std::string, fs::path> soundByName;   // the lower-case .wav name: its spelling
    for (const fs::path &file : files)
    {
        if (lowerUtf8(file.extension()) == kWavExtension)
        {
            soundByName[lowerUtf8(file)] = file;
        }
        else if (isConvertedSound(file))
        {
            const fs::path wav = withWavExtension(file);
            soundByName.emplace(lowerUtf8(wav), wav);
        }
    }
    std::vector<fs::path> sounds;
    for (const auto &[name, sound] : soundByName)
    {
        sounds.push_back(sound);
    }
    std::sort(sounds.begin(), sounds.end());
    return sounds;
}

std::vector<fs::path> listGameSounds(const fs::path &folder)
{
    std::vector<fs::path> files;
    std::error_code ec;
    if (!fs::is_directory(folder, ec))
    {
        return files;
    }
    for (const fs::directory_entry &entry : fs::directory_iterator(folder, ec))
    {
        if (entry.is_regular_file(ec))
        {
            files.push_back(entry.path());
        }
    }
    return gameSoundNames(files);
}

}  // namespace argon
