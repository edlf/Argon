// Argon: song and sound files, see song_files.hpp.
#include "audio/song_files.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <optional>
#include <string>

#include "dr_mp3.h"
#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"

namespace argon
{
namespace
{
constexpr int kVorbisReadFrames = 4096;   // frames decoded per stb_vorbis call
constexpr size_t kMaxFrameCountDigits = 18;   // fits size_t

std::string lowerText(std::string text)
{
    for (auto &character : text)
    {
        character = char(std::tolower(static_cast<unsigned char>(character)));
    }
    return text;
}

std::string lowerExtension(const std::filesystem::path &path)
{
    const std::u8string utf8Extension = path.extension().u8string();
    return lowerText(std::string(utf8Extension.begin(), utf8Extension.end()));
}

std::optional<size_t> parseFrameCount(const std::string &text)
{
    if (text.empty() || text.size() > kMaxFrameCountDigits)
    {
        return std::nullopt;
    }
    size_t value = 0;
    for (const char digit : text)
    {
        if (digit < '0' || digit > '9')
        {
            return std::nullopt;
        }
        value = value * 10 + size_t(digit - '0');
    }
    return value;
}

// the number after "<key>=" in the first comment with that key (any case)
std::optional<size_t> commentFrameCount(const std::vector<std::string> &comments, const std::string &lowerKey)
{
    const std::string prefix = lowerKey + "=";
    for (const std::string &comment : comments)
    {
        if (lowerText(comment.substr(0, prefix.size())) == prefix)
        {
            return parseFrameCount(comment.substr(prefix.size()));
        }
    }
    return std::nullopt;
}

bool readVorbisSamples(stb_vorbis *stream, Pcm &out)
{
    const stb_vorbis_info info = stb_vorbis_get_info(stream);
    if (info.channels <= 0)
    {
        return false;
    }
    std::vector<short> block(size_t(kVorbisReadFrames) * size_t(info.channels));
    out.samples.clear();
    int frames = 0;
    do
    {
        frames = stb_vorbis_get_samples_short_interleaved(stream, info.channels, block.data(), int(block.size()));
        out.samples.insert(out.samples.end(), block.begin(), block.begin() + std::ptrdiff_t(std::max(frames, 0)) * info.channels);
    } while (frames > 0);
    out.channels = info.channels;
    out.rate = int(info.sample_rate);
    return !out.samples.empty();
}
}  // namespace

std::vector<std::string> vorbisComments(stb_vorbis *stream)
{
    const stb_vorbis_comment comment = stb_vorbis_get_comment(stream);
    std::vector<std::string> comments;
    for (int i = 0; i < comment.comment_list_length; ++i)
    {
        comments.emplace_back(comment.comment_list[i]);
    }
    return comments;
}

bool isSongFile(const std::filesystem::path &path)
{
    const std::string extension = lowerExtension(path);
    return extension == ".wav" || extension == ".flac" || extension == ".ogg" || extension == ".mp3";
}

bool isMp3File(const std::filesystem::path &path)
{
    return lowerExtension(path) == ".mp3";
}

SampleLoop sampleLoopFromComments(const std::vector<std::string> &comments)
{
    const std::optional<size_t> start = commentFrameCount(comments, "loopstart");
    const std::optional<size_t> length = commentFrameCount(comments, "looplength");
    if (!start || !length || *length == 0)
    {
        return {};
    }
    return {*start, *start + *length};
}

size_t playFramesFromComments(const std::vector<std::string> &comments, size_t frames)
{
    const std::optional<size_t> playLength = commentFrameCount(comments, "playlength");
    return playLength && *playLength < frames ? *playLength : 0;
}

bool decodeMp3(const std::vector<uint8_t> &file, Pcm &out)
{
    drmp3_config format{};
    drmp3_uint64 frames = 0;
    drmp3_int16 *samples = drmp3_open_memory_and_read_pcm_frames_s16(file.data(), file.size(), &format, &frames, nullptr);
    if (!samples || format.channels == 0 || frames == 0)
    {
        drmp3_free(samples, nullptr);
        return false;
    }
    out.channels = int(format.channels);
    out.rate = int(format.sampleRate);
    out.samples.assign(samples, samples + size_t(frames) * format.channels);
    drmp3_free(samples, nullptr);
    return true;
}

bool decodeOggVorbis(const std::vector<uint8_t> &file, Pcm &out)
{
    int error = 0;
    stb_vorbis *stream = stb_vorbis_open_memory(file.data(), int(file.size()), &error, nullptr);
    if (!stream)
    {
        return false;
    }
    const bool decoded = readVorbisSamples(stream, out);
    const std::vector<std::string> comments = vorbisComments(stream);
    const SampleLoop loop = sampleLoopFromComments(comments);
    out.sampleLoopBegin = loop.begin;
    out.sampleLoopEnd = loop.end;
    out.playFrames = playFramesFromComments(comments, out.samples.size() / size_t(std::max(out.channels, 1)));
    stb_vorbis_close(stream);
    return decoded;
}

}  // namespace argon
