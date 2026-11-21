// Argon: the Xbox's HRTF on the game's 3D voices (docs\audio\mix-levels.md "Mix levels"). The game links DirectSound's full HRTF
// (DirectSoundUseFullHRTF): 2222 Sensaura filters of 31 eight-bit taps and an interaural delay at 0x1459d8 in
// default.xbe, picked through a 61 x 31 azimuth / elevation index at 0x156f98 (0x13dd6c).
// DirectSound hands the bytes to the VP as they are; the VP keeps 12-bit taps, so each byte is a code: sign, 3-bit
// exponent, 4-bit mantissa (decodeTap). Read that way the front filter #1170, the one DirectSound loads for a voice
// whose 3D processing is off, is close to flat. xemu reads the bytes as
// two's complement and normalises each ear to a sum of |taps| of 1, which turns every filter into a ragged, dull one.
// The VP runs each voice through its left and right filter, the delay on the far ear, parameters gliding per sample.
#pragma once
#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include <glm/glm.hpp>

namespace argon::hrtf
{

constexpr int kXboxTaps = 31;
constexpr int kMaxTaps = 512;   // the longest ears: MIT KEMAR's at its longest tap choice (kemar.hpp)

struct Tables
{
    struct Filter
    {
        std::array<uint8_t, kXboxTaps> codes{};   // decodeTap
        int delay = 0;   // samples at 48 kHz
    };
    std::vector<Filter> filters;
    std::vector<uint16_t> index;   // [azimuth / 3][(elevation + 90) / 6], 61 x 31
    bool empty() const
    {
        return filters.empty() || index.empty();
    }
};
// where the tables are: <game>\audio\hrtf.bin (tools\assets\extract_hrtf.py) first, else the USA default.xbe itself
struct TableFiles
{
    std::filesystem::path extracted, xbe;
};
// throws std::runtime_error when neither file holds them
Tables loadTables(const TableFiles &files);
// a tap's code as the VP's 12-bit tap: exponent 0 holds the mantissa as it is, exponent e > 0 (16 + mantissa) <<
// (e - 1), up to 1984, of 2048 (s.11). No document gives the scale, nor whether the crosstalk mixbins' 1-bit
// headroom also applies to the HRTF bins; 1024 played every 3D sound (the engines too) 6 dB louder than a real Xbox
// next to the 2D speaker-file sounds (the pickups), 2048 matches it (the front pair: -7.5 dB)
float decodeTap(uint8_t code);

struct Head
{
    glm::vec3 position{0.f};
    glm::vec3 front{0.f, 0.f, 1.f}, top{0.f, 1.f, 0.f}, right{1.f, 0.f, 0.f};
};
struct Direction
{
    float azimuth = 0.f, elevation = 0.f;   // degrees: azimuth + to the right, 180 behind; elevation + up
    float distance = 0.f;                   // from the head; 0: on it (a NOPOSITION voice sits at the listener)
};
// DirectSound's HRTF: the source's direction from the head (on the head: straight ahead)
Direction directionOf(const Head &head, const glm::vec3 &source);
// a source this far straight ahead (the viewer's own engine, which the game places at its craft ahead of the camera)
constexpr float kAheadDistance = 1.f;
// the filter pick (0x13dd6c) in surround: a source behind takes its mirror's filters in front
Direction mirroredToFront(const Direction &direction);

// a 3D voice's share of the front and rear 3D mixbins (amplitudes)
struct FrontRear
{
    float front = 1.f, rear = 0.f;
};
// DirectSound's front / rear split in surround (in stereo the rear is silent): full front until 45 degrees off the
// front axis, then towards the rear, power-preserving; inside 0.5 m it crossfades to equal halves at the head
FrontRear surroundGains(const Direction &direction);

struct Ears
{
    std::array<float, kMaxTaps> left{}, right{};   // taps past `taps` are 0
    int taps = kXboxTaps;
    float delay = 0.f;   // samples: > 0 delays the left ear, < 0 the right
};
// DirectSound's filter pick (0x13dd6c) and load (stereo speakers: no rear folding); without tables, a pass-through
Ears earsFor(const Tables &tables, const Direction &direction);
// both ears hear the sound as it is (xemu with its HRTF option off)
Ears passThroughEars();

// one voice's filter, on the audio thread: interleaved stereo in, both ears out. Each ear is a delay line, then the
// ears' taps over the delayed sound, a block at a time (fir_block.hpp); the work follows the ears' own tap counts,
// never kMaxTaps. The VP's per-sample glide of every tap is a crossfade from where the taps were to the target (the
// same sum, two filters), which ends once the old taps weigh under kSettledWeight
class EarFilter
{
  public:
    void setTarget(const Ears &ears);   // glides there, as the VP's per-sample smoothing
    void jumpTo(const Ears &ears);      // no glide (a voice's first parameters)
    void process(const float *in, float *out, size_t frames);

  private:
    static constexpr int kMaxDelay = 42;
    static constexpr int kDelayLine = kMaxDelay + 2;   // the whole delay and the sample after it
    static constexpr float kSettledWeight = 1e-5f;     // -100 dB
    static constexpr int kBlockFrames = 64;            // fir::kBlockFrames
    struct BlockWeights   // from_'s share at each frame of a block
    {
        std::array<float, kBlockFrames> from{};
        int frames = 0;
    };
    void processBlock(const float *in, float *out, int frames);
    void glide();
    float delayed(int ear, float sample, float delay);
    void filterEar(int ear, const BlockWeights &weights, float *out);
    void startGlideFromCurrentTaps();

    Ears from_, target_;
    float fromWeight_ = 0.f;   // from_'s share of the taps, (1 - glide)^samples since the target changed
    float delay_ = 0.f;
    std::array<std::array<float, kDelayLine>, 2> delayLines_{};
    // oldest first: the kMaxTaps delayed samples before the block, then the block's
    std::array<std::array<float, kMaxTaps + kBlockFrames>, 2> history_{};
    int delayAt_ = 0;
};

}  // namespace argon::hrtf
