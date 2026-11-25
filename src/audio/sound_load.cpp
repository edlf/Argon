#include "audio/sound_load.hpp"

#include <cctype>
#include <cstring>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <vector>

#include "audio/song_files.hpp"
#include "files/game_path.hpp"

#include "dr_flac.h"

namespace argon
{

namespace fs = std::filesystem;

std::string lower(std::string text)
{
    for (auto &character : text)
    {
        character = char(std::tolower(unsigned(character)));
    }
    return text;
}

// the cache key: the lower-case UTF-8 path (song names may hold any characters)
std::string pathKey(const fs::path &path)
{
    std::u8string utf8 = path.u8string();
    return lower(std::string(utf8.begin(), utf8.end()));
}

Pcm loadSound(const fs::path &path)
{
    Pcm out;
    std::ifstream f(findGamePath(path), std::ios::binary);
    std::string name = pathKey(path);
    if (!f)
    {
        throw std::runtime_error("cannot read " + name);
    }
    std::vector<uint8_t> d((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    if (d.size() >= 4 && !std::memcmp(d.data(), "fLaC", 4))
    {
        unsigned int ch = 0;
        drflac_uint32 rate = 0;
        drflac_uint64 frames = 0;
        drflac_int16 *samples = drflac_open_memory_and_read_pcm_frames_s16(d.data(), d.size(), &ch, &rate, &frames, nullptr);
        if (!samples || !ch)
        {
            throw std::runtime_error(name + ": bad FLAC");
        }
        out.channels = int(ch);
        out.rate = int(rate);
        out.samples.assign(samples, samples + size_t(frames) * ch);
        drflac_free(samples, nullptr);
    }
    else if (d.size() >= 4 && !std::memcmp(d.data(), "OggS", 4))
    {
        if (!decodeOggVorbis(d, out))
        {
            throw std::runtime_error(name + ": bad Ogg Vorbis");
        }
    }
    else if (isMp3File(path))
    {
        if (!decodeMp3(d, out))
        {
            throw std::runtime_error(name + ": bad MP3");
        }
    }
    else
    {
        return loadWav(path.string());
    }
    if (out.channels > 2)
    {   // the voices are mono / stereo: keep the front pair
        std::vector<int16_t> st;
        size_t frames = out.samples.size() / size_t(out.channels);
        st.reserve(frames * 2);
        for (size_t i = 0; i < frames; ++i)
        {
            st.push_back(out.samples[i * out.channels]);
            st.push_back(out.samples[i * out.channels + 1]);
        }
        out.samples.swap(st);
        out.channels = 2;
    }
    return out;
}

}  // namespace argon
