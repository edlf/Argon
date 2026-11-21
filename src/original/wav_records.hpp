// The game's .wav sounds byte for byte: RIFF WAVE files of chunks ("fmt ", "fact", "smpl", "data", each a
// RiffChunkHeader then its body, padded to even), in Xbox ADPCM or PCM16. Field meanings: docs\audio\README.md.
#pragma once
#include <cstddef>
#include <cstdint>

namespace argon::original
{

constexpr uint16_t kWavPcm = 1;
constexpr uint16_t kWavXboxAdpcm = 0x69;   // 36-byte blocks of 64 samples per channel

#pragma pack(push, 1)

struct RiffHeader
{
    char riff[4];                 // "RIFF"
    uint32_t size;
    char wave[4];                 // "WAVE"
};

struct RiffChunkHeader
{
    char id[4];
    uint32_t size;                // the body's, without the pad byte
};

// "fmt "
struct WavFormat
{
    uint16_t formatTag;           // kWavPcm / kWavXboxAdpcm
    uint16_t channels;
    uint32_t samplesPerSecond;
    uint32_t averageBytesPerSecond;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
};

// "fact"
struct WavFact
{
    uint32_t sampleCount;         // per channel
};

struct WavSampleLoop
{
    uint32_t cuePointId;
    uint32_t type;
    uint32_t start;
    uint32_t end;                 // inclusive
    uint32_t fraction;
    uint32_t playCount;
};

// "smpl", up to its first loop
struct WavSampler
{
    uint32_t manufacturer, product, samplePeriod, midiUnityNote, midiPitchFraction, smpteFormat, smpteOffset;
    uint32_t loopCount;
    uint32_t samplerDataSize;
    WavSampleLoop firstLoop;
};

#pragma pack(pop)

static_assert(sizeof(RiffHeader) == 12 && sizeof(RiffChunkHeader) == 8 && sizeof(WavFormat) == 16);
static_assert(offsetof(WavFormat, blockAlign) == 12 && sizeof(WavSampleLoop) == 24 && sizeof(WavSampler) == 60);
static_assert(offsetof(WavSampler, firstLoop.start) == 44 && offsetof(WavSampler, firstLoop.end) == 48);

} // namespace argon::original
