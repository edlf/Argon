// Argon: which story scene plays when, and the speech lines' keys (movie\story_scenes.hpp, movie\speech_lines.hpp)
#include "movie/story_scenes.hpp"

#include <algorithm>
#include <cstring>

#include "movie/speech_lines.hpp"

namespace argon::story
{

namespace
{
// the names in 0x1b4158's file names, per pilot
const char *const kStoryNames[kPilots] = {
    "tara", "yuri", "kumiko", "zaki", "jordan", "marcus", "leah", "kahekili", "alicia", "vladimir", "ami", "pharaoh", "kiri", "jaya", "max", "kutoo"};
// 0x9b170's folder names (0x1a29cc, 0x1a29bc, ... 0x1a2964)
const char *const kCharacterFolders[kPilots] = {
    "tara", "yuri", "kumiko", "darius", "jordan", "marcus", "leah", "kukailimoku", "alicia", "vladimir", "abi", "zafir", "kaitlyn", "jaya", "lukas", "kutoo"};
constexpr size_t kComparedBytes = 5;
constexpr int kAmateur = 1, kMaster = 3, kRedshift = 4;
constexpr int kOtherRoute = 8;

bool belowExclamation(char character)
{
    return static_cast<unsigned char>(character) < '!';
}

// the first five bytes of each, the terminating NUL counting
bool samePrefix(const std::string &text, const char *prefix)
{
    char padded[kComparedBytes]{};
    std::memcpy(padded, text.c_str(), std::min(text.size() + 1, kComparedBytes));
    return std::strncmp(padded, prefix, kComparedBytes) == 0;
}
} // namespace

std::string storyName(int pilot)
{
    return kStoryNames[std::clamp(pilot, 0, kPilots - 1)];
}

std::string preRaceScene(const std::string &trackFolder, int pilot)
{
    return trackFolder + "_" + storyName(pilot) + ".nms";
}

std::optional<std::string> raceEndScene(int level, int track, const std::string &trackFolder)
{
    if (level < kAmateur || level > kRedshift || track < 0 || track >= kRouteTracks)
    {
        return std::nullopt;
    }
    if (level <= kMaster)
    {
        return trackFolder + "_" + storyName(track ^ kOtherRoute) + "_beatnemesis.nms";
    }
    return trackFolder + "_" + storyName(track) + "_winning.nms";
}

int pilotOfFolder(const std::string &folder)
{
    for (int pilot = 0; pilot < kPilots; ++pilot)
    {
        if (samePrefix(folder, kCharacterFolders[pilot]))
        {
            return pilot;
        }
    }
    return -1;
}

// 0x9aff0: compare while the object's key has characters above ' ' and the line's none below '!'
bool speechKeysMatch(const std::string &objectKey, const char *lineKey)
{
    if (objectKey.empty() || belowExclamation(objectKey[0]))
    {
        return true;
    }
    for (size_t i = 0; i < objectKey.size() && !belowExclamation(objectKey[i]); ++i)
    {
        if (belowExclamation(lineKey[i]))
        {
            return true;
        }
        if (objectKey[i] != lineKey[i])
        {
            return false;
        }
    }
    return true;
}

std::optional<SpeechLine> speechLineFor(const std::string &key)
{
    for (const SpeechLine &line : kSpeechLines)
    {
        if (speechKeysMatch(key, line.key))
        {
            return line;
        }
    }
    return std::nullopt;
}

} // namespace argon::story
