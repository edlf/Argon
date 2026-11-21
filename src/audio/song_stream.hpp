// One sound playing songs back to back: miniaudio's audio thread moves from a song to the queued one in the middle of
// a read, so the junction is exact to the sample and the resampler runs on through it (a sound per song lost frames
// there, and miniaudio 0.11.21 starts scheduled sounds only on a mixing call's boundary). Silence while nothing is
// queued, and where a song's feed runs short (its filler held up). Songs come as SongFeeds in the music's format.
// The audio thread reads only the song it plays and the queued one's pointer: the caller keeps a queued song alive
// until switches() has counted the stream moving on to it, and the one before it until the stream moves past that.
#pragma once
#include <cstdint>
#include <memory>

#include "audio/song_feed.hpp"

namespace argon
{

class AudioOutput;

class SongStream
{
  public:
    static constexpr int kChannels = SongFeed::kChannels, kRate = SongFeed::kRate;

    explicit SongStream(AudioOutput &output);   // playing (silence) at once; ok() false without a device
    ~SongStream();
    SongStream(const SongStream &) = delete;
    SongStream &operator=(const SongStream &) = delete;

    bool ok() const;
    void queue(SongFeed &song);   // the song after the one playing; only once switches() has counted the last one
    // the queued song swapped for another; false when the stream has already moved on to `queued`
    bool requeue(SongFeed &queued, SongFeed &song);
    uint32_t switches() const;     // how many queued songs the stream has moved on to
    void setVolume(float volume);
    void pause();
    void resume();

  private:
    struct Source;
    std::unique_ptr<Source> source_;
};

}  // namespace argon
