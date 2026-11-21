// Argon: custom music. On the Xbox, Music Source = Custom plays the songs ripped to the console's soundtracks
// (0x440e70: 0x4c-byte entries, name + song count; entry 0 is the game's own music), picked in FE LEVELMUSIC "Edit
// Playlist" (ui\playlist.cpp); the playlist (32 x (soundtrack, song), 0x4406fc) is saved with the profile
// (0xbf350 / 0xbf710). The viewer reads <saves>\custom_music: each folder is a soundtrack, its .wav / .flac / .ogg /
// .mp3 files the songs (files directly in custom_music form a soundtrack called "custom_music").
// docs\frontend\options.md "Custom music".
#pragma once
#include <filesystem>
#include <string>
#include <vector>

struct Soundtrack
{
    std::u16string name;
    std::vector<std::filesystem::path> songs;   // in name order
};

// a song of a soundtrack, by their places in the lists
struct SongRef
{
    int soundtrack = 0, song = 0;
    bool operator==(const SongRef &other) const = default;
};

class CustomMusic
{
  public:
    static constexpr size_t kPlaylistSlots = 32;

    void setFolder(const std::filesystem::path &folder);
    // the folder read again, at most once a second (secondsNow: any steady clock); the playlist stays on the same songs
    void refresh(double secondsNow);

    bool available() const;   // the game's check (0x2c240): more than one soundtrack, its own counting as one
    bool customChosen() const;   // Music Source = Custom
    void chooseCustom(bool custom);

    const std::vector<SongRef> &playlist() const;
    std::vector<SongRef> songsNotInPlaylist() const;   // soundtrack by soundtrack (0x27b80)
    bool playlistFull() const;
    void addToPlaylist(const SongRef &song);
    void removeFromPlaylist(size_t index);   // the playlist kept packed (a song taken out closes the gap, 0x28210)
    std::vector<std::filesystem::path> playlistFiles() const;

    const std::filesystem::path &file(const SongRef &song) const;
    std::u16string label(const SongRef &song) const;   // "%s\%s": the soundtrack (at most 31 characters), the song

    // the profile's "playlist": "soundtrack\song|..." by name, so it survives songs added to the folder
    std::string playlistString() const;
    void readPlaylistString(const std::string &text);

  private:
    void scan();
    bool holds(const SongRef &song) const;

    std::filesystem::path folder_;
    std::vector<Soundtrack> soundtracks_;
    std::vector<SongRef> playlist_;
    bool customChosen_ = false;
    double lastRefresh_ = -10.0;
};
