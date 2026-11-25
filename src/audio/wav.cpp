// WAV files: Xbox ADPCM (format 0x69) and PCM16 decoding, PCM16 writing (docs/audio/README.md).
#include "wav.hpp"
#include "audio/adpcm_chunk_ends.hpp"
#include "files/game_path.hpp"
#include "original/record_read.hpp"
#include "original/wav_records.hpp"

#include <algorithm>
#include <stdexcept>
#include <cstdio>
#include <cstring>

namespace argon
{

// ------------------------------------------------------------------ Xbox ADPCM
static const int kStep[89] = {7,    8,    9,    10,   11,    12,    13,    14,    16,    17,    19,    21,    23,    25,    28,    31,    34,   37,
                              41,   45,   50,   55,   60,    66,    73,    80,    88,    97,    107,   118,   130,   143,   157,   173,   190,  209,
                              230,  253,  279,  307,  337,   371,   408,   449,   494,   544,   598,   658,   724,   796,   876,   963,   1060, 1166,
                              1282, 1411, 1552, 1707, 1878,  2066,  2272,  2499,  2749,  3024,  3327,  3660,  4026,  4428,  4871,  5358,  5894, 6484,
                              7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767};
static const int kIndex[16] = {-1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8};

static inline int16_t imaNibble(int nib, int &pred, int &idx)
{
    int step = kStep[idx];
    int diff = step >> 3;
    if (nib & 1)
    {
        diff += step >> 2;
    }
    if (nib & 2)
    {
        diff += step >> 1;
    }
    if (nib & 4)
    {
        diff += step;
    }
    pred += (nib & 8) ? -diff : diff;
    pred = std::clamp(pred, -32768, 32767);
    idx = std::clamp(idx + kIndex[nib], 0, 88);
    return int16_t(pred);
}

// Xbox ADPCM: per block and channel a 4-byte header (int16 sample, u8 step index, u8 0),
// then 4-byte groups interleaved per channel, 8 groups (= 64 nibbles) per channel per block.
// A block holds 64 samples per channel: the header sample followed by the first 63 nibbles
// (the 64th nibble is padding). Checked against the files: this layout joins blocks seamlessly
// and matches the `fact` sample counts; dropping the header sample puts a step at every block
// (a 345 Hz buzz at 22 kHz). The last sample of every 128th block is the encoder's mistake (adpcm_chunk_ends.hpp).
static void decodeXboxAdpcm(const uint8_t *bytes, size_t size, int ch, size_t blockAlign, std::vector<int16_t> &out)
{
    const int spb = int(kAdpcmBlockFrames);
    int16_t decoded[8][64];
    for (size_t b = 0; b + blockAlign <= size; b += blockAlign)
    {
        const uint8_t *blk = bytes + b;
        int pred[8], idx[8];
        for (int c = 0; c < ch; ++c)
        {
            pred[c] = int16_t(blk[c * 4] | blk[c * 4 + 1] << 8);
            idx[c] = std::clamp(int(blk[c * 4 + 2]), 0, 88);
            decoded[c][0] = int16_t(pred[c]);
        }
        const uint8_t *data = blk + 4 * ch;
        for (int g = 0; g < 8; ++g)
        {
            for (int c = 0; c < ch; ++c)
            {
                const uint8_t *nibbles = data + (size_t(g) * ch + c) * 4;
                for (int k = 0; k < 4; ++k)
                {
                    int nibble = g * 8 + k * 2;   // nibble number 0..63 -> output sample s + 1
                    int16_t lo = imaNibble(nibbles[k] & 15, pred[c], idx[c]);
                    int16_t hi = imaNibble(nibbles[k] >> 4, pred[c], idx[c]);
                    if (nibble + 1 < spb)
                    {
                        decoded[c][nibble + 1] = lo;
                    }
                    if (nibble + 2 < spb)
                    {
                        decoded[c][nibble + 2] = hi;
                    }
                }
            }
        }
        size_t base = out.size();
        out.resize(base + size_t(spb) * ch);
        for (int s = 0; s < spb; ++s)
        {
            for (int c = 0; c < ch; ++c)
            {
                out[base + size_t(s) * ch + c] = decoded[c][s];
            }
        }
    }
}

// ------------------------------------------------------------------ WAV
using original::readAt;
using original::RiffChunkHeader;

static bool isChunk(const RiffChunkHeader &chunk, const char (&id)[5])
{
    return std::memcmp(chunk.id, id, 4) == 0;
}

static void putChunkHeader(FILE *file, const char (&id)[5], uint32_t size)
{
    RiffChunkHeader chunk;
    std::memcpy(chunk.id, id, 4);
    chunk.size = size;
    std::fwrite(&chunk, sizeof chunk, 1, file);
}

Pcm loadWav(const std::string &path)
{
    Pcm out;
    std::vector<uint8_t> bytes;
    if (!readFile(path, bytes))
    {
        throw std::runtime_error("cannot read " + path);
    }
    const auto riff = readAt<original::RiffHeader>(bytes, 0);
    if (bytes.size() < sizeof riff || std::memcmp(riff.riff, "RIFF", 4) || std::memcmp(riff.wave, "WAVE", 4))
    {
        throw std::runtime_error(path + ": not a WAV");
    }
    original::WavFormat format{};
    uint32_t factSamples = 0, smplBegin = 0, smplEnd = 0;
    const uint8_t *data = nullptr;
    size_t dataSize = 0;
    for (size_t o = sizeof riff; o + sizeof(RiffChunkHeader) <= bytes.size();)
    {
        const auto chunk = readAt<RiffChunkHeader>(bytes, o);
        const size_t body = o + sizeof chunk;
        size_t avail = std::min<size_t>(chunk.size, bytes.size() - body);
        if (isChunk(chunk, "fmt ") && avail >= sizeof(original::WavFormat))
        {
            format = readAt<original::WavFormat>(bytes, body);
        }
        else if (isChunk(chunk, "fact") && avail >= sizeof(original::WavFact))
        {
            factSamples = readAt<original::WavFact>(bytes, body).sampleCount;
        }
        else if (isChunk(chunk, "smpl") && avail >= sizeof(original::WavSampler))
        {
            const auto loop = readAt<original::WavSampler>(bytes, body).firstLoop;
            smplBegin = loop.start;
            smplEnd = loop.end;
        }
        else if (isChunk(chunk, "data"))
        {
            data = bytes.data() + body;
            dataSize = avail;
        }
        o += sizeof chunk + chunk.size + (chunk.size & 1);
    }
    const uint16_t fmt = format.formatTag, ch = format.channels, ba = format.blockAlign, bits = format.bitsPerSample;
    const uint32_t rate = format.samplesPerSecond;
    if (!data || !ch || ch > 8 || !rate)
    {
        throw std::runtime_error(path + ": missing fmt/data");
    }
    out = Pcm{};
    out.channels = ch;
    out.rate = int(rate);
    if (smplEnd > smplBegin)
    {
        out.sampleLoopBegin = smplBegin;
        out.sampleLoopEnd = size_t(smplEnd) + 1;
    }
    if (fmt == original::kWavPcm && bits == 16)
    {
        out.samples.resize(dataSize / 2);
        std::memcpy(out.samples.data(), data, out.samples.size() * 2);
    }
    else if (fmt == original::kWavXboxAdpcm && ba == 36 * ch)
    {
        decodeXboxAdpcm(data, dataSize, ch, ba, out.samples);
        repairAdpcmChunkEnds(out.samples, ch);
        // Don't trim to `fact` here: looping sounds wrap on whole 64-sample blocks (measured:
        // the full-block seam is smooth, the fact-trimmed seam clicks up to ~3x a normal step).
        // One-shots stop at playFrames instead.
        if (factSamples && size_t(factSamples) * ch < out.samples.size())
        {
            out.playFrames = factSamples;
        }
    }
    else
    {
        throw std::runtime_error(path + ": unsupported WAV format " + std::to_string(fmt));
    }
    return out;
}

bool writeWav(const std::string &path, const Pcm &pcm)
{
    FILE *file = std::fopen(path.c_str(), "wb");
    if (!file)
    {
        return false;
    }
    const uint32_t dataBytes = uint32_t(pcm.samples.size() * 2);
    original::WavFormat format{};
    format.formatTag = original::kWavPcm;
    format.channels = uint16_t(pcm.channels);
    format.samplesPerSecond = uint32_t(pcm.rate);
    format.blockAlign = uint16_t(2 * format.channels);
    format.averageBytesPerSecond = format.samplesPerSecond * format.blockAlign;
    format.bitsPerSample = 16;
    original::RiffHeader riff;
    std::memcpy(riff.riff, "RIFF", 4);
    riff.size = uint32_t(sizeof riff.wave + 2 * sizeof(RiffChunkHeader) + sizeof format) + dataBytes;
    std::memcpy(riff.wave, "WAVE", 4);
    std::fwrite(&riff, sizeof riff, 1, file);
    putChunkHeader(file, "fmt ", sizeof format);
    std::fwrite(&format, sizeof format, 1, file);
    putChunkHeader(file, "data", dataBytes);
    std::fwrite(pcm.samples.data(), 2, pcm.samples.size(), file);
    std::fclose(file);
    return true;
}

} // namespace argon
