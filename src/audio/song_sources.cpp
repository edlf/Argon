// Argon: the song files read a chunk at a time (audio\song_sources.hpp).
#include "audio/song_sources.hpp"

#include <algorithm>
#include <cstring>
#include <limits>

#include "audio/song_files.hpp"
#include "dr_flac.h"
#include "dr_mp3.h"
#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"

namespace argon
{
namespace
{

// a decoder over the file's bytes: its frames up to the song's length, the front pair of more than two channels
class CompressedFrames : public SongFrames
{
  public:
    explicit CompressedFrames(std::vector<uint8_t> bytes) : bytes_(std::move(bytes))
    {
    }
    bool valid() const
    {
        return sourceChannels_ > 0 && rate_ > 0;
    }
    int channels() const override
    {
        return std::min(sourceChannels_, 2);
    }
    int rate() const override
    {
        return rate_;
    }
    size_t read(int16_t *out, size_t frames) override
    {
        frames = std::min(frames, framesLeft_);
        const size_t made = sourceChannels_ <= 2 ? decode(out, frames) : decodeFrontPair(out, frames);
        framesLeft_ -= made;
        return made;
    }

  protected:
    virtual size_t decode(int16_t *out, size_t frames) = 0;   // interleaved, all of the source's channels

    std::vector<uint8_t> bytes_;   // the decoder reads these: never changed after it opens them
    int sourceChannels_ = 0, rate_ = 0;
    size_t framesLeft_ = std::numeric_limits<size_t>::max();

  private:
    size_t decodeFrontPair(int16_t *out, size_t frames)
    {
        scratch_.resize(frames * size_t(sourceChannels_));
        const size_t made = decode(scratch_.data(), frames);
        for (size_t f = 0; f < made; ++f)
        {
            out[f * 2] = scratch_[f * size_t(sourceChannels_)];
            out[f * 2 + 1] = scratch_[f * size_t(sourceChannels_) + 1];
        }
        return made;
    }

    std::vector<int16_t> scratch_;
};

class FlacFrames final : public CompressedFrames
{
  public:
    explicit FlacFrames(std::vector<uint8_t> bytes) : CompressedFrames(std::move(bytes))
    {
        flac_ = drflac_open_memory(bytes_.data(), bytes_.size(), nullptr);
        if (flac_)
        {
            sourceChannels_ = int(flac_->channels);
            rate_ = int(flac_->sampleRate);
        }
    }
    ~FlacFrames() override
    {
        drflac_close(flac_);
    }

  private:
    size_t decode(int16_t *out, size_t frames) override
    {
        return size_t(drflac_read_pcm_frames_s16(flac_, frames, out));
    }

    drflac *flac_ = nullptr;
};

class Mp3Frames final : public CompressedFrames
{
  public:
    explicit Mp3Frames(std::vector<uint8_t> bytes) : CompressedFrames(std::move(bytes))
    {
        opened_ = drmp3_init_memory(&mp3_, bytes_.data(), bytes_.size(), nullptr);
        if (opened_)
        {
            sourceChannels_ = int(mp3_.channels);
            rate_ = int(mp3_.sampleRate);
        }
    }
    ~Mp3Frames() override
    {
        if (opened_)
        {
            drmp3_uninit(&mp3_);
        }
    }

  private:
    size_t decode(int16_t *out, size_t frames) override
    {
        return size_t(drmp3_read_pcm_frames_s16(&mp3_, frames, out));
    }

    drmp3 mp3_{};
    bool opened_ = false;
};

// the converted game sounds' PLAYLENGTH comment (the .wav's `fact`) ends it early, as decodeOggVorbis does
class VorbisFrames final : public CompressedFrames
{
  public:
    explicit VorbisFrames(std::vector<uint8_t> bytes) : CompressedFrames(std::move(bytes))
    {
        int error = 0;
        vorbis_ = stb_vorbis_open_memory(bytes_.data(), int(bytes_.size()), &error, nullptr);
        if (!vorbis_)
        {
            return;
        }
        const stb_vorbis_info info = stb_vorbis_get_info(vorbis_);
        sourceChannels_ = info.channels;
        rate_ = int(info.sample_rate);
        const size_t playFrames = playFramesFromComments(vorbisComments(vorbis_), stb_vorbis_stream_length_in_samples(vorbis_));
        if (playFrames)
        {
            framesLeft_ = playFrames;
        }
    }
    ~VorbisFrames() override
    {
        if (vorbis_)
        {
            stb_vorbis_close(vorbis_);
        }
    }

  private:
    size_t decode(int16_t *out, size_t frames) override
    {
        const int made = stb_vorbis_get_samples_short_interleaved(vorbis_, sourceChannels_, out, int(frames) * sourceChannels_);
        return size_t(std::max(made, 0));
    }

    stb_vorbis *vorbis_ = nullptr;
};

bool startsWith(const std::vector<uint8_t> &bytes, const char (&magic)[5])
{
    return bytes.size() >= 4 && std::memcmp(bytes.data(), magic, 4) == 0;
}

template <class Frames> std::unique_ptr<SongFrames> opened(std::vector<uint8_t> bytes)
{
    auto frames = std::make_unique<Frames>(std::move(bytes));
    if (!frames->valid())
    {
        return nullptr;
    }
    return frames;
}

}  // namespace

std::unique_ptr<SongFrames> compressedSongFrames(const std::filesystem::path &name, std::vector<uint8_t> bytes)
{
    if (startsWith(bytes, "fLaC"))
    {
        return opened<FlacFrames>(std::move(bytes));
    }
    if (startsWith(bytes, "OggS"))
    {
        return opened<VorbisFrames>(std::move(bytes));
    }
    if (isMp3File(name))
    {
        return opened<Mp3Frames>(std::move(bytes));
    }
    return nullptr;
}

}  // namespace argon
