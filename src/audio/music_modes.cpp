#include "audio/music_modes.hpp"

#include "audio/game_sound_files.hpp"

#include <algorithm>
#include <cctype>
#include <map>
#include <string>

namespace argon
{
namespace
{
namespace fs = std::filesystem;

std::string lowerCase(std::string text)
{
    for (char &character : text)
    {
        character = char(std::tolower(static_cast<unsigned char>(character)));
    }
    return text;
}

bool allDigits(const std::string &text)
{
    for (const char character : text)
    {
        if (!std::isdigit(static_cast<unsigned char>(character)))
        {
            return false;
        }
    }
    return !text.empty();
}

// "<folder><n>" -> n; 0 when the name is not a segment's
int segmentNumber(const std::string &stem, const std::string &folder)
{
    if (stem.size() <= folder.size() || stem.compare(0, folder.size(), folder) != 0)
    {
        return 0;
    }
    const std::string digits = stem.substr(folder.size());
    return allDigits(digits) ? std::stoi(digits) : 0;
}

void addFile(MusicFolder &folder, std::map<int, fs::path> &segments, const fs::path &file, const std::string &folderName)
{
    const std::string stem = lowerCase(file.stem().string());
    if (stem == "intro")
    {
        folder.intro = file;
    }
    else if (stem == "on_start")
    {
        folder.onStart = file;
    }
    else if (stem == "on_start_second_lap")
    {
        folder.secondLap = file;
    }
    else if (const int segment = segmentNumber(stem, folderName); segment > 0)
    {
        segments[segment] = file;
    }
}
}  // namespace

MusicFolder MusicFolder::read(const fs::path &dir)
{
    MusicFolder folder;
    std::map<int, fs::path> segments;
    const std::string folderName = lowerCase(dir.filename().string());
    for (const fs::path &file : listGameSounds(dir))
    {
        addFile(folder, segments, file, folderName);
    }
    for (const auto &[number, file] : segments)
    {
        folder.segments.push_back(file);
    }
    return folder;
}

std::vector<fs::path> MusicFolder::playlist() const
{
    std::vector<fs::path> songs = {intro, onStart, secondLap};
    songs.insert(songs.end(), segments.begin(), segments.end());
    return songs;
}

MusicModes::MusicModes(const MusicFolder &folder)
    : hasIntro_(!folder.intro.empty()), hasOnStart_(!folder.onStart.empty()), hasSecondLap_(!folder.secondLap.empty()), segments_(int(folder.segments.size()))
{
}

void MusicModes::set(int mode)
{
    if (mode == Intro && !hasIntro_)
    {
        mode = OnStart;
    }
    if (mode == OnStart && !hasOnStart_)
    {
        mode = FirstSegment;
    }
    if (mode == SecondLap)
    {
        if (hasSecondLap_)
        {
            afterStinger_ = mode_;
            mode_ = SecondLap;
        }
        return;
    }
    if (mode == Finale)
    {
        mode_ = segments_ ? Finale : SecondLap;
        return;
    }
    mode_ = mode;
    afterStinger_ = mode < FirstSegment ? int(FirstSegment) : mode;
}

void MusicModes::stepUp()
{
    if (mode_ > SecondLap && mode_ < 7)
    {
        set(mode_ + 1);
    }
}

void MusicModes::stepDown()
{
    if (mode_ > FirstSegment)
    {
        set(mode_ - 1);
    }
}

void MusicModes::playFinale()
{
    mode_ = Finale;
}

void MusicModes::startOver(int mode)
{
    restart(0);
    set(Intro);
    set(mode);
}

void MusicModes::cycle()
{
    set(mode_ > SecondLap && mode_ < 6 ? mode_ + 1 : int(FirstSegment));
}

void MusicModes::restart(size_t)
{
    mode_ = FirstSegment;
    afterStinger_ = FirstSegment;
    lastStarted_ = -1;
    playing_ = 0;
}

bool MusicModes::repeatsTheSong() const
{
    return mode_ == lastStarted_ && mode_ >= FirstSegment;
}

// 0xdf350: modes 3..7 are the segments 1..5, the highest one a folder of n segments reaches being n - 1; mode 8 the
// last segment
size_t MusicModes::songOf(int mode) const
{
    if (mode < FirstSegment)
    {
        return size_t(mode);
    }
    if (segments_ == 0)
    {
        return FirstSegment;
    }
    const int lastSegment = segments_ - 1;
    const int segment = mode == Finale ? lastSegment : std::clamp(mode - int(FirstSegment), 0, std::max(0, lastSegment - 1));
    return size_t(FirstSegment + segment);
}

size_t MusicModes::next() const
{
    return repeatsTheSong() ? playing_ : songOf(mode_);
}

void MusicModes::started(size_t song)
{
    playing_ = song;
    if (repeatsTheSong())
    {
        cycle();
        return;
    }
    lastStarted_ = mode_;
    if (mode_ == OnStart || mode_ == SecondLap)
    {
        mode_ = afterStinger_;
    }
}

}  // namespace argon
