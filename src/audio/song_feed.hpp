// A song decoded a few seconds ahead of where it plays instead of whole (a whole song held ~40 MB): the song's frames
// (SongFrames, read once from the start: the file decoders in song_sources.hpp, or a decoded Pcm) are converted to the
// music's format, kChannels x kRate 16-bit, into a ring the audio thread reads (SongStream).
// One thread fills a feed (fill), one other reads it (read / finished): the ring's ends are atomics, no lock.
#pragma once
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "audio/wav.hpp"

namespace argon
{

// a song's frames in its own format, read once from the start
class SongFrames
{
  public:
    virtual ~SongFrames() = default;
    virtual int channels() const = 0;   // 1 or 2
    virtual int rate() const = 0;
    virtual size_t read(int16_t *out, size_t frames) = 0;   // interleaved; fewer than asked only at the end
};

// a decoded song (a WAV, or a test's): its playFrames (`fact`) if set, else all of it
class PcmFrames final : public SongFrames
{
  public:
    explicit PcmFrames(std::shared_ptr<const Pcm> pcm);
    int channels() const override;
    int rate() const override;
    size_t read(int16_t *out, size_t frames) override;

  private:
    std::shared_ptr<const Pcm> pcm_;
    size_t cursor_ = 0, length_ = 0;
};

class SongFeed
{
  public:
    static constexpr int kChannels = 2, kRate = 44100;   // the game's music: PCM16 stereo 44.1 kHz

    explicit SongFeed(std::unique_ptr<SongFrames> frames);   // ok() false when its format cannot be converted
    ~SongFeed();
    SongFeed(const SongFeed &) = delete;
    SongFeed &operator=(const SongFeed &) = delete;

    bool ok() const;
    // the filling thread: decodes until the ring is full or the song has ended
    void fill();
    // the reading thread: up to `frames` frames (kChannels interleaved); fewer when the ring runs short
    size_t read(int16_t *out, size_t frames);
    bool finished() const;   // the reading thread: decoded to its end and all read

  private:
    struct Converter;
    void decodeChunk();
    void convert(const int16_t *in, size_t frames);
    void flushConversion();
    void write(const int16_t *frames, size_t count);
    size_t freeFrames() const;

    std::unique_ptr<SongFrames> frames_;
    std::unique_ptr<Converter> converter_;   // null: the song is already in the music's format
    size_t chunkOutFrames_ = 0;              // the most frames one decoded chunk can make
    std::vector<int16_t> chunk_, converted_;
    uint64_t sourceFrames_ = 0, writtenFrames_ = 0;
    std::vector<int16_t> ring_;
    std::atomic<uint64_t> writeEnd_{0}, readEnd_{0};   // frames written / read since the start
    std::atomic<bool> decoded_{false};
};

// the viewer's thread that keeps the playing and the queued song filled (tests fill them from MusicSequence::update)
class SongFeedFiller
{
  public:
    SongFeedFiller();
    ~SongFeedFiller();
    SongFeedFiller(const SongFeedFiller &) = delete;
    SongFeedFiller &operator=(const SongFeedFiller &) = delete;

    void keepFilled(std::shared_ptr<SongFeed> playing, std::shared_ptr<SongFeed> queued);

  private:
    void run();

    std::mutex lock_;
    std::condition_variable wake_;
    std::shared_ptr<SongFeed> playing_, queued_;
    bool stopping_ = false;
    std::thread thread_;
};

}  // namespace argon
