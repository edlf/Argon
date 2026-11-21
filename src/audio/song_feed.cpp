// Argon: a song decoded a few seconds ahead (audio\song_feed.hpp).
#include "audio/song_feed.hpp"

#include <algorithm>
#include <chrono>
#include <cstring>

#include "miniaudio.h"

// defined MA_API by miniaudio (the length ma_convert_frames gives a converted song) but not declared in its header
extern "C" ma_uint64 ma_calculate_frame_count_after_resampling(ma_uint32 sampleRateOut, ma_uint32 sampleRateIn, ma_uint64 frameCountIn);

namespace argon
{
namespace
{
constexpr size_t kRingFrames = size_t(1) << 19;   // ~11.9 s of the music's format (2 MB); the filler tops it up every 20 ms
constexpr size_t kChunkFrames = 4096;              // the song's frames decoded at a time
constexpr size_t kFlushFrames = 64;                // silence fed to the converter at the end, at a time
constexpr auto kFillPeriod = std::chrono::milliseconds(20);

bool inMusicFormat(const SongFrames &frames)
{
    return frames.channels() == SongFeed::kChannels && frames.rate() == SongFeed::kRate;
}
}  // namespace

// ------------------------------------------------------------------ a decoded song
PcmFrames::PcmFrames(std::shared_ptr<const Pcm> pcm) : pcm_(std::move(pcm))
{
    const size_t all = pcm_->samples.size() / size_t(std::max(pcm_->channels, 1));
    length_ = pcm_->playFrames ? std::min(all, pcm_->playFrames) : all;
}

int PcmFrames::channels() const
{
    return pcm_->channels;
}

int PcmFrames::rate() const
{
    return pcm_->rate;
}

size_t PcmFrames::read(int16_t *out, size_t frames)
{
    const size_t count = std::min(frames, length_ - cursor_);
    std::memcpy(out, pcm_->samples.data() + cursor_ * size_t(pcm_->channels), count * size_t(pcm_->channels) * sizeof(int16_t));
    cursor_ += count;
    return count;
}

// ------------------------------------------------------------------ the feed
// the song's rate and channels to the music's (the same conversion ma_convert_frames made of a whole song)
struct SongFeed::Converter
{
    ma_data_converter converter{};
    bool ready = false;

    explicit Converter(const SongFrames &frames)
    {
        const ma_data_converter_config config = ma_data_converter_config_init(
            ma_format_s16, ma_format_s16, ma_uint32(frames.channels()), kChannels, ma_uint32(frames.rate()), kRate);
        ready = ma_data_converter_init(&config, nullptr, &converter) == MA_SUCCESS;
    }
    ~Converter()
    {
        if (ready)
        {
            ma_data_converter_uninit(&converter, nullptr);
        }
    }
};

SongFeed::SongFeed(std::unique_ptr<SongFrames> frames) : frames_(std::move(frames)), ring_(kRingFrames * kChannels)
{
    chunk_.resize(kChunkFrames * size_t(std::max(frames_->channels(), 1)));
    chunkOutFrames_ = kChunkFrames;
    if (!inMusicFormat(*frames_) && frames_->rate() > 0)
    {
        converter_ = std::make_unique<Converter>(*frames_);
        chunkOutFrames_ = size_t(ma_calculate_frame_count_after_resampling(kRate, ma_uint32(frames_->rate()), kChunkFrames)) + kFlushFrames;
        converted_.resize(chunkOutFrames_ * kChannels);
    }
}

SongFeed::~SongFeed() = default;

bool SongFeed::ok() const
{
    const bool convertible = inMusicFormat(*frames_) || (converter_ && converter_->ready);
    return convertible && chunkOutFrames_ <= kRingFrames;
}

void SongFeed::fill()
{
    while (ok() && !decoded_.load(std::memory_order_relaxed) && freeFrames() >= chunkOutFrames_)
    {
        decodeChunk();
    }
}

void SongFeed::decodeChunk()
{
    const size_t framesRead = frames_->read(chunk_.data(), kChunkFrames);
    sourceFrames_ += framesRead;
    if (!converter_)
    {
        write(chunk_.data(), framesRead);
    }
    else
    {
        convert(chunk_.data(), framesRead);
    }
    if (framesRead < kChunkFrames)
    {
        flushConversion();
        decoded_.store(true, std::memory_order_release);
    }
}

void SongFeed::convert(const int16_t *in, size_t frames)
{
    const size_t inChannels = size_t(frames_->channels());
    while (frames > 0)
    {
        ma_uint64 consumed = frames, made = chunkOutFrames_;
        ma_data_converter_process_pcm_frames(&converter_->converter, in, &consumed, converted_.data(), &made);
        write(converted_.data(), size_t(made));
        if (consumed == 0 && made == 0)
        {
            return;
        }
        in += size_t(consumed) * inChannels;
        frames -= size_t(consumed);
    }
}

// the converter holds back a few frames: silence after the song brings them out, up to the song's converted length
void SongFeed::flushConversion()
{
    if (!converter_)
    {
        return;
    }
    const uint64_t length = ma_calculate_frame_count_after_resampling(kRate, ma_uint32(frames_->rate()), sourceFrames_);
    const std::vector<int16_t> silence(kFlushFrames * size_t(frames_->channels()), 0);
    for (int attempt = 0; writtenFrames_ < length && attempt < 64; ++attempt)
    {
        ma_uint64 consumed = kFlushFrames, made = chunkOutFrames_;
        ma_data_converter_process_pcm_frames(&converter_->converter, silence.data(), &consumed, converted_.data(), &made);
        write(converted_.data(), size_t(std::min<uint64_t>(made, length - writtenFrames_)));
    }
}

// fill() leaves room for a whole chunk; a converter giving more than its expected count is cut, never overwrites
void SongFeed::write(const int16_t *frames, size_t count)
{
    count = std::min(count, freeFrames());
    const uint64_t end = writeEnd_.load(std::memory_order_relaxed);
    const size_t at = size_t(end % kRingFrames);
    const size_t first = std::min(count, kRingFrames - at);
    std::memcpy(ring_.data() + at * kChannels, frames, first * kChannels * sizeof(int16_t));
    std::memcpy(ring_.data(), frames + first * kChannels, (count - first) * kChannels * sizeof(int16_t));
    writtenFrames_ += count;
    writeEnd_.store(end + count, std::memory_order_release);
}

size_t SongFeed::freeFrames() const
{
    return kRingFrames - size_t(writeEnd_.load(std::memory_order_relaxed) - readEnd_.load(std::memory_order_acquire));
}

size_t SongFeed::read(int16_t *out, size_t frames)
{
    const uint64_t start = readEnd_.load(std::memory_order_relaxed);
    const size_t available = std::min(frames, size_t(writeEnd_.load(std::memory_order_acquire) - start));
    const size_t at = size_t(start % kRingFrames);
    const size_t first = std::min(available, kRingFrames - at);
    std::memcpy(out, ring_.data() + at * kChannels, first * kChannels * sizeof(int16_t));
    std::memcpy(out + first * kChannels, ring_.data(), (available - first) * kChannels * sizeof(int16_t));
    readEnd_.store(start + available, std::memory_order_release);
    return available;
}

bool SongFeed::finished() const
{
    return decoded_.load(std::memory_order_acquire) && readEnd_.load(std::memory_order_relaxed) == writeEnd_.load(std::memory_order_acquire);
}

// ------------------------------------------------------------------ the filling thread
SongFeedFiller::SongFeedFiller() : thread_(&SongFeedFiller::run, this)
{
}

SongFeedFiller::~SongFeedFiller()
{
    {
        std::lock_guard<std::mutex> guard(lock_);
        stopping_ = true;
    }
    wake_.notify_one();
    thread_.join();
}

void SongFeedFiller::keepFilled(std::shared_ptr<SongFeed> playing, std::shared_ptr<SongFeed> queued)
{
    std::lock_guard<std::mutex> guard(lock_);
    playing_ = std::move(playing);
    queued_ = std::move(queued);
}

// every kFillPeriod: the two songs topped up, outside the lock (each kept alive by its own reference meanwhile)
void SongFeedFiller::run()
{
    std::unique_lock<std::mutex> guard(lock_);
    while (!stopping_)
    {
        wake_.wait_for(guard, kFillPeriod);
        std::shared_ptr<SongFeed> playing = playing_, queued = queued_;
        guard.unlock();
        for (SongFeed *feed : {playing.get(), queued.get()})
        {
            if (feed)
            {
                feed->fill();
            }
        }
        guard.lock();
    }
}

}  // namespace argon
