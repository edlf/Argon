#include "audio/song_stream.hpp"

#include <algorithm>
#include <atomic>
#include <cstring>

#include "audio/audio_output.hpp"
#include "miniaudio.h"

namespace argon
{

// a miniaudio data source (its base first, as miniaudio casts the source to it) and the sound that plays it
struct SongStream::Source
{
    ma_data_source_base base{};
    ma_sound sound{};
    bool baseReady = false, soundReady = false;
    SongFeed *playing = nullptr;   // the audio thread's
    std::atomic<SongFeed *> queued{nullptr};
    std::atomic<uint32_t> switches{0};

    ~Source()
    {
        if (soundReady)
        {
            ma_sound_uninit(&sound);   // first: no more reads after it
        }
        if (baseReady)
        {
            ma_data_source_uninit(&base);
        }
    }

    // the audio thread: the playing song's frames, then the queued song's, silence when neither is left or the playing
    // one's feed is short (it goes on from there)
    static ma_result read(ma_data_source *source, void *framesOut, ma_uint64 frameCount, ma_uint64 *framesRead)
    {
        auto *stream = reinterpret_cast<Source *>(source);
        auto *out = static_cast<int16_t *>(framesOut);
        ma_uint64 done = 0;
        while (done < frameCount)
        {
            if (stream->playing)
            {
                done += stream->playing->read(out + done * kChannels, size_t(frameCount - done));
            }
            if (done == frameCount)
            {
                break;
            }
            const bool ended = !stream->playing || stream->playing->finished();
            if (!ended || !stream->takeQueued())
            {
                break;
            }
        }
        std::memset(out + done * kChannels, 0, size_t(frameCount - done) * kChannels * sizeof(int16_t));
        *framesRead = frameCount;
        return MA_SUCCESS;
    }

    bool takeQueued()
    {
        SongFeed *next = queued.exchange(nullptr);
        if (!next)
        {
            return false;
        }
        playing = next;
        switches.fetch_add(1);
        return true;
    }

    static ma_result dataFormat(ma_data_source *, ma_format *format, ma_uint32 *channels, ma_uint32 *sampleRate, ma_channel *map, size_t mapCapacity)
    {
        *format = ma_format_s16;
        *channels = kChannels;
        *sampleRate = kRate;
        ma_channel_map_init_standard(ma_standard_channel_map_default, map, mapCapacity, kChannels);
        return MA_SUCCESS;
    }

    static ma_result notSeekable(ma_data_source *, ma_uint64)
    {
        return MA_NOT_IMPLEMENTED;
    }
    static ma_result noCursor(ma_data_source *, ma_uint64 *cursor)
    {
        *cursor = 0;
        return MA_NOT_IMPLEMENTED;
    }
    static ma_result endless(ma_data_source *, ma_uint64 *length)
    {
        *length = 0;
        return MA_NOT_IMPLEMENTED;
    }
};

SongStream::SongStream(AudioOutput &output) : source_(std::make_unique<Source>())
{
    static ma_data_source_vtable functions = {Source::read, Source::notSeekable, Source::dataFormat, Source::noCursor, Source::endless, nullptr, 0};
    if (!output.engine())
    {
        return;
    }
    ma_data_source_config config = ma_data_source_config_init();
    config.vtable = &functions;
    source_->baseReady = ma_data_source_init(&config, &source_->base) == MA_SUCCESS;
    if (!source_->baseReady)
    {
        return;
    }
    // the music stream's 2D voice is on the front speakers (WorldEffects' front pair)
    ma_sound_config sound = ma_sound_config_init_2(output.engine());
    sound.pDataSource = &source_->base;
    sound.flags = MA_SOUND_FLAG_NO_SPATIALIZATION;
    sound.channelsOut = kChannels;
    sound.pInitialAttachment = output.group(Bus::Main, VoicePath::Front);
    source_->soundReady = ma_sound_init_ex(output.engine(), &sound, &source_->sound) == MA_SUCCESS;
    if (source_->soundReady)
    {
        ma_sound_start(&source_->sound);
    }
}

SongStream::~SongStream() = default;

bool SongStream::ok() const
{
    return source_->soundReady;
}

void SongStream::queue(SongFeed &song)
{
    source_->queued.store(&song);
}

bool SongStream::requeue(SongFeed &queued, SongFeed &song)
{
    SongFeed *expected = &queued;
    return source_->queued.compare_exchange_strong(expected, &song);
}

uint32_t SongStream::switches() const
{
    return source_->switches.load();
}
void SongStream::setVolume(float volume)
{
    if (ok())
    {
        ma_sound_set_volume(&source_->sound, volume);
    }
}

void SongStream::pause()
{
    if (ok())
    {
        ma_sound_stop(&source_->sound);
    }
}

void SongStream::resume()
{
    if (ok())
    {
        ma_sound_start(&source_->sound);
    }
}

}  // namespace argon
