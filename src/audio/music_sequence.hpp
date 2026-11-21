// A playlist played back to back without a gap (the race music's <t>_musicN.wav are bar-aligned parts of one piece):
// the songs go through one SongStream, the next one opened (its first seconds decoded) on a worker while the current
// one plays, both kept filled by a SongFeedFiller; a PlayOrder picks which (the folder's order, or the game's music
// modes). Used for the race's music and the front end's.
#pragma once
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <future>
#include <memory>
#include <string>
#include <vector>

#include "audio/play_order.hpp"
#include "audio/song_stream.hpp"
#include "audio/wav.hpp"

namespace argon
{

class AudioOutput;

class MusicSequence
{
  public:
    using SongOpener = std::shared_ptr<SongFeed> (*)(const std::filesystem::path &);   // any thread; null: unreadable

    // policy: std::launch::async in the viewer (a worker opens, a thread keeps the songs filled), deferred in tests
    // (opened when taken, filled by update)
    MusicSequence(AudioOutput &output, SongOpener open, std::launch policy = std::launch::async);
    ~MusicSequence()
    {
        stop();
    }
    MusicSequence(const MusicSequence &) = delete;
    MusicSequence &operator=(const MusicSequence &) = delete;

    void setPlaylist(std::vector<std::filesystem::path> files);   // stops; played in order, looping
    // stops; `order` (kept by the caller while this plays) picks each next song
    void setPlaylist(std::vector<std::filesystem::path> files, PlayOrder &order);
    bool hasSongs() const
    {
        return !files_.empty();
    }
    void start();   // the song the order picks next
    void stop();
    bool playing() const
    {
        return stream_ != nullptr;
    }
    void update();   // every frame: follows the stream, queues (or replaces) the song the order picks next
    void setVolume(float volume);
    void pause();
    void resume();
    std::string nowPlaying() const;   // the song's file name; empty while none plays

  private:
    static constexpr size_t kNone = size_t(-1);

    void startOpening(size_t index);
    void collectRead();
    void requestRead(size_t index);
    bool offer(size_t index);
    void followStream();
    void keepSongsFilled();

    AudioOutput &output_;
    SongOpener open_;
    std::launch policy_;
    std::unique_ptr<SongFeedFiller> filler_;   // async policy only
    std::vector<std::filesystem::path> files_;
    FolderOrder folderOrder_;
    PlayOrder *order_ = &folderOrder_;
    std::shared_ptr<SongFeed> playingSong_, queuedSong_;   // kept alive for the stream (before it: it goes first)
    std::shared_ptr<SongFeed> readySong_;                  // opened, not queued (a feed plays once)
    size_t playingIndex_ = kNone, queuedIndex_ = kNone, readyIndex_ = kNone, unreadableIndex_ = kNone;
    uint32_t switchesSeen_ = 0;
    std::unique_ptr<SongStream> stream_;
    std::future<std::shared_ptr<SongFeed>> reading_;
    size_t readingIndex_ = 0;
    float volume_ = 1.f;
};

}  // namespace argon
