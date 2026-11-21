// Argon: custom music (custom_music.hpp).
#include "audio/custom_music.hpp"

#include "audio/song_files.hpp"
#include "files/utf.hpp"

#include <algorithm>

namespace
{
namespace fs = std::filesystem;

std::u16string pathName(const fs::path &path)
{
    const std::u8string name = path.u8string();
    return utf16(std::string(name.begin(), name.end()));
}

std::vector<fs::path> songsIn(const fs::path &folder)
{
    std::vector<fs::path> songs;
    std::error_code error;
    for (const auto &entry : fs::directory_iterator(folder, error))
    {
        if (entry.is_regular_file(error) && argon::isSongFile(entry.path()))
        {
            songs.push_back(entry.path());
        }
    }
    std::sort(songs.begin(), songs.end());
    return songs;
}

std::vector<fs::path> foldersIn(const fs::path &folder)
{
    std::vector<fs::path> folders;
    std::error_code error;
    for (const auto &entry : fs::directory_iterator(folder, error))
    {
        if (entry.is_directory(error))
        {
            folders.push_back(entry.path());
        }
    }
    std::sort(folders.begin(), folders.end());
    return folders;
}
}  // namespace

void CustomMusic::setFolder(const std::filesystem::path &folder)
{
    folder_ = folder;
    lastRefresh_ = -10.0;
}

void CustomMusic::refresh(double secondsNow)
{
    if (!soundtracks_.empty() && secondsNow - lastRefresh_ < 1.0 && secondsNow >= lastRefresh_)
    {
        return;
    }
    lastRefresh_ = secondsNow;
    const std::string kept = playlistString();
    scan();
    readPlaylistString(kept);
}

void CustomMusic::scan()
{
    soundtracks_.clear();
    if (folder_.empty())
    {
        return;
    }
    std::vector<fs::path> loose = songsIn(folder_);
    if (!loose.empty())
    {
        soundtracks_.push_back({u"custom_music", std::move(loose)});
    }
    for (const fs::path &folder : foldersIn(folder_))
    {
        std::vector<fs::path> songs = songsIn(folder);
        if (!songs.empty())
        {
            soundtracks_.push_back({pathName(folder.filename()), std::move(songs)});
        }
    }
}

bool CustomMusic::available() const
{
    return !soundtracks_.empty();
}

bool CustomMusic::customChosen() const
{
    return customChosen_;
}

void CustomMusic::chooseCustom(bool custom)
{
    customChosen_ = custom;
}

const std::vector<SongRef> &CustomMusic::playlist() const
{
    return playlist_;
}

std::vector<SongRef> CustomMusic::songsNotInPlaylist() const
{
    std::vector<SongRef> songs;
    for (int soundtrack = 0; soundtrack < int(soundtracks_.size()); ++soundtrack)
    {
        for (int song = 0; song < int(soundtracks_[size_t(soundtrack)].songs.size()); ++song)
        {
            const SongRef ref{soundtrack, song};
            if (std::find(playlist_.begin(), playlist_.end(), ref) == playlist_.end())
            {
                songs.push_back(ref);
            }
        }
    }
    return songs;
}

bool CustomMusic::playlistFull() const
{
    return playlist_.size() >= kPlaylistSlots;
}

void CustomMusic::addToPlaylist(const SongRef &song)
{
    if (!playlistFull() && holds(song))
    {
        playlist_.push_back(song);
    }
}

void CustomMusic::removeFromPlaylist(size_t index)
{
    if (index < playlist_.size())
    {
        playlist_.erase(playlist_.begin() + std::ptrdiff_t(index));
    }
}

std::vector<std::filesystem::path> CustomMusic::playlistFiles() const
{
    std::vector<fs::path> files;
    for (const SongRef &song : playlist_)
    {
        files.push_back(file(song));
    }
    return files;
}

const std::filesystem::path &CustomMusic::file(const SongRef &song) const
{
    return soundtracks_[size_t(song.soundtrack)].songs[size_t(song.song)];
}

std::u16string CustomMusic::label(const SongRef &song) const
{
    return soundtracks_[size_t(song.soundtrack)].name.substr(0, 31) + u"\\" + pathName(file(song).stem());
}

bool CustomMusic::holds(const SongRef &song) const
{
    return song.soundtrack >= 0 && song.soundtrack < int(soundtracks_.size()) && song.song >= 0 &&
           song.song < int(soundtracks_[size_t(song.soundtrack)].songs.size());
}

std::string CustomMusic::playlistString() const
{
    std::string text;
    for (const SongRef &song : playlist_)
    {
        if (!holds(song))
        {
            continue;
        }
        if (!text.empty())
        {
            text += "|";
        }
        text += utf8(soundtracks_[size_t(song.soundtrack)].name) + "\\" + utf8(pathName(file(song).filename()));
    }
    return text;
}

void CustomMusic::readPlaylistString(const std::string &text)
{
    playlist_.clear();
    size_t at = 0;
    while (at <= text.size() && !playlistFull())
    {
        size_t end = text.find('|', at);
        if (end == std::string::npos)
        {
            end = text.size();
        }
        const std::string entry = text.substr(at, end - at);
        const size_t split = entry.rfind('\\');
        if (split != std::string::npos)
        {
            for (size_t soundtrack = 0; soundtrack < soundtracks_.size(); ++soundtrack)
            {
                if (utf8(soundtracks_[soundtrack].name) != entry.substr(0, split))
                {
                    continue;
                }
                for (size_t song = 0; song < soundtracks_[soundtrack].songs.size(); ++song)
                {
                    if (utf8(pathName(soundtracks_[soundtrack].songs[song].filename())) == entry.substr(split + 1))
                    {
                        playlist_.push_back({int(soundtrack), int(song)});
                    }
                }
            }
        }
        at = end + 1;
    }
}
