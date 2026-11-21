// Argon: the game's HRTF filters, see hrtf.hpp.
#include "audio/hrtf.hpp"

#include <algorithm>
#include <stdexcept>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iterator>

#include "audio/fir_block.hpp"

namespace argon::hrtf
{
namespace
{
constexpr uint32_t kFiltersVa = 0x1459d8, kIndexVa = 0x156f98;
constexpr size_t kFilterCount = 2222, kFilterBytes = 32;
constexpr int kAzimuthSteps = 61, kElevationSteps = 31;   // 0..180 in 3 degrees, -90..90 in 6
constexpr float kGlide = 0.01f;                           // xemu's HRTF_PARAM_SMOOTH_ALPHA, per sample

uint32_t readU32(const std::vector<uint8_t> &xbe, size_t at)
{
    uint32_t v = 0;
    std::memcpy(&v, xbe.data() + at, sizeof v);
    return v;
}

// the file offset of a virtual address, through the XBE's section headers; 0 when no section holds `bytes` there
size_t fileOffsetOf(const std::vector<uint8_t> &xbe, uint32_t va, size_t bytes)
{
    constexpr size_t kBaseAddress = 0x104, kSectionCount = 0x11c, kSectionHeaders = 0x120, kSectionHeaderBytes = 56;
    if (xbe.size() < kSectionHeaders + 4)
    {
        return 0;
    }
    const uint32_t base = readU32(xbe, kBaseAddress), count = readU32(xbe, kSectionCount);
    const size_t headers = size_t(readU32(xbe, kSectionHeaders) - base);
    for (uint32_t s = 0; s < count && headers + (s + 1) * kSectionHeaderBytes <= xbe.size(); ++s)
    {
        const size_t h = headers + s * kSectionHeaderBytes;
        const uint32_t sectionVa = readU32(xbe, h + 4), raw = readU32(xbe, h + 12), rawSize = readU32(xbe, h + 16);
        if (va >= sectionVa && va + bytes <= sectionVa + rawSize && raw + (va - sectionVa) + bytes <= xbe.size())
        {
            return raw + (va - sectionVa);
        }
    }
    return 0;
}

const float *tapsOf(const Ears &ears, int ear)
{
    return ear == 0 ? ears.left.data() : ears.right.data();
}

// the same taps (each ear's past its count are 0) and delay
bool sameEars(const Ears &first, const Ears &second)
{
    return first.taps == second.taps && first.delay == second.delay && std::equal(first.left.begin(), first.left.begin() + first.taps, second.left.begin()) &&
           std::equal(first.right.begin(), first.right.begin() + first.taps, second.right.begin());
}

// DirectSound's ftoi: the FPU's round to nearest
int ftoi(float x)
{
    return int(std::nearbyint(x));
}

std::array<float, kMaxTaps> earOf(const Tables::Filter &filter)
{
    std::array<float, kMaxTaps> ear{};
    for (size_t k = 0; k < filter.codes.size(); ++k)
    {
        ear[k] = decodeTap(filter.codes[k]);
    }
    return ear;
}

// the filter pick (0x13dd6c): elevation in 6 degree steps, azimuth in 3, 6 or 12 degree steps by elevation
// DirectSound's power to millibels (1000 log10 x mB, DSBVOLUME_MIN at 0: silence) as the VP's amplitude
float powerToGain(float power)
{
    constexpr float kMbPerDecade = 1000.f, kMbPerAmplitudeDecade = 2000.f;
    if (power <= 0.f)
    {
        return 0.f;
    }
    if (power >= 1.f)
    {
        return 1.f;
    }
    return std::pow(10.f, float(ftoi(kMbPerDecade * std::log10(power))) / kMbPerAmplitudeDecade);
}

size_t filterIndex(const Tables &tables, const Direction &direction)
{
    constexpr int kInnerElevation = 30, kOuterElevation = 60;
    const int elevation = direction.elevation >= 0.f ? ftoi(direction.elevation + 3.f) / 6 * 6 : ftoi(direction.elevation - 3.f) / 6 * 6;
    const int absElevation = std::abs(elevation);
    const float absAzimuth = std::abs(direction.azimuth);
    int azimuth = 0;
    if (absElevation == 90)
    {
        azimuth = 0;
    }
    else if (absElevation > kOuterElevation)
    {
        azimuth = ftoi(absAzimuth + 6.f) / 12 * 12;
    }
    else if (absElevation > kInnerElevation)
    {
        azimuth = ftoi(absAzimuth + 3.f) / 6 * 6;
    }
    else
    {
        azimuth = ftoi(absAzimuth + 1.5f) / 3 * 3;
    }
    int row = azimuth / 3, column = (elevation + 90) / 6;
    row = row < kAzimuthSteps ? row : 0;
    column = column < kElevationSteps ? column : 0;
    return std::min<size_t>(tables.index[size_t(row * kElevationSteps + column)], tables.filters.size() - 2);
}
// the tables' bytes in a file: <game>\audio\hrtf.bin (tools\assets\extract_hrtf.py) or default.xbe; null when they are not there
struct TableBytes
{
    const uint8_t *filters = nullptr, *index = nullptr;
};

TableBytes extractedTableBytes(const std::vector<uint8_t> &data)
{
    constexpr size_t kHeaderBytes = 16;
    constexpr uint32_t kVersion = 1;
    const size_t indexCount = size_t(kAzimuthSteps * kElevationSteps);
    if (data.size() != kHeaderBytes + kFilterCount * kFilterBytes + indexCount * sizeof(uint16_t) || std::memcmp(data.data(), "ARHR", 4) != 0 ||
        readU32(data, 4) != kVersion || readU32(data, 8) != kFilterCount || readU32(data, 12) != indexCount)
    {
        return {};
    }
    return TableBytes{data.data() + kHeaderBytes, data.data() + kHeaderBytes + kFilterCount * kFilterBytes};
}

TableBytes xbeTableBytes(const std::vector<uint8_t> &data)
{
    const size_t filters = fileOffsetOf(data, kFiltersVa, kFilterCount * kFilterBytes);
    const size_t index = fileOffsetOf(data, kIndexVa, size_t(kAzimuthSteps * kElevationSteps) * sizeof(uint16_t));
    if (!filters || !index)
    {
        return {};
    }
    return TableBytes{data.data() + filters, data.data() + index};
}

std::vector<uint8_t> readFile(const std::filesystem::path &path)
{
    std::ifstream file(path, std::ios::binary);
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

Tables tablesFrom(const TableBytes &bytes)
{
    Tables tables;
    tables.filters.resize(kFilterCount);
    for (size_t f = 0; f < kFilterCount; ++f)
    {
        const uint8_t *row = bytes.filters + f * kFilterBytes;
        std::memcpy(tables.filters[f].codes.data(), row, kXboxTaps);
        tables.filters[f].delay = row[kXboxTaps];
    }
    tables.index.resize(size_t(kAzimuthSteps * kElevationSteps));
    std::memcpy(tables.index.data(), bytes.index, tables.index.size() * sizeof(uint16_t));
    return tables;
}

bool indexInRange(const Tables &tables)
{
    for (uint16_t filter : tables.index)
    {
        if (filter + 1u >= kFilterCount)
        {
            return false;
        }
    }
    return true;
}
}  // namespace

Tables loadTables(const TableFiles &files)
{
    std::vector<uint8_t> data = readFile(files.extracted);
    TableBytes bytes = extractedTableBytes(data);
    std::filesystem::path source = files.extracted;
    if (!bytes.filters)
    {
        data = readFile(files.xbe);
        bytes = xbeTableBytes(data);
        source = files.xbe;
    }
    if (!bytes.filters)
    {
        throw std::runtime_error("no HRTF tables in " + files.extracted.string() + " (tools/assets/extract_hrtf.py) or " + files.xbe.string() +
                                 " (0x1459d8 / 0x156f98)");
    }
    Tables tables = tablesFrom(bytes);
    if (!indexInRange(tables))
    {
        throw std::runtime_error(source.string() + ": the HRTF index points past its filters (not the USA XBE?)");
    }
    return tables;
}

Direction directionOf(const Head &head, const glm::vec3 &source)
{
    const glm::vec3 offset = source - head.position;
    const float distance = glm::length(offset);
    if (distance <= 0.f)
    {
        return {};
    }
    const glm::vec3 towards = offset / distance;
    const float alongFront = glm::dot(towards, head.front), alongRight = glm::dot(towards, head.right);
    const float alongTop = glm::dot(towards, head.top);
    const float absTop = std::abs(alongTop), flat = std::sqrt(alongRight * alongRight + alongFront * alongFront);
    Direction direction;
    direction.elevation = absTop < flat ? absTop / flat * 45.f : 90.f - flat / absTop * 45.f;
    direction.elevation = alongTop < 0.f ? -direction.elevation : direction.elevation;
    const float absFront = std::abs(alongFront), absRight = std::abs(alongRight);
    if (absFront > absRight)
    {
        direction.azimuth = absRight / absFront * 45.f;
    }
    else if (absRight > 0.f)
    {
        direction.azimuth = 90.f - absFront / absRight * 45.f;
    }
    direction.azimuth = alongFront < 0.f ? 180.f - direction.azimuth : direction.azimuth;
    direction.azimuth = alongRight < 0.f ? -direction.azimuth : direction.azimuth;
    direction.distance = distance;
    return direction;
}

Direction mirroredToFront(const Direction &direction)
{
    constexpr float kSide = 90.f, kBehind = 180.f;
    Direction mirrored = direction;
    const float absAzimuth = std::abs(direction.azimuth);
    if (absAzimuth > kSide)
    {
        mirrored.azimuth = std::copysign(kBehind - absAzimuth, direction.azimuth);
    }
    return mirrored;
}

FrontRear surroundGains(const Direction &direction)
{
    constexpr float kCrossfadeDistance = 0.5f, kQuarter = 90.f;
    float factor = 0.5f * (1.f + (std::abs(direction.azimuth) / kQuarter - 1.f) * (1.f - std::abs(direction.elevation) / kQuarter));
    factor = std::clamp(factor * 2.f - 0.5f, 0.f, 1.f);
    if (direction.distance < kCrossfadeDistance)
    {
        factor = 0.5f + (factor - 0.5f) * direction.distance / 2.f;
    }
    return {powerToGain(1.f - factor), powerToGain(factor)};
}

Ears earsFor(const Tables &tables, const Direction &direction)
{
    if (tables.empty())
    {
        return passThroughEars();
    }
    Ears ears;
    const size_t first = filterIndex(tables, direction);
    const bool toTheRight = direction.azimuth >= 0.f;
    const Tables::Filter &left = tables.filters[toTheRight ? first : first + 1];
    const Tables::Filter &right = tables.filters[toTheRight ? first + 1 : first];
    ears.left = earOf(left);
    ears.right = earOf(right);
    ears.delay = toTheRight ? float(left.delay) : -float(right.delay);
    return ears;
}

float decodeTap(uint8_t code)
{
    constexpr uint8_t kSign = 0x80, kExponentMask = 7, kMantissaMask = 0xf;
    constexpr int kMantissaBits = 4, kHiddenBit = 16;
    constexpr float kUnity = 2048.f;
    const int exponent = (code >> kMantissaBits) & kExponentMask, mantissa = code & kMantissaMask;
    const int magnitude = exponent == 0 ? mantissa : (kHiddenBit + mantissa) << (exponent - 1);
    return float((code & kSign) ? -magnitude : magnitude) / kUnity;
}

Ears passThroughEars()
{
    Ears ears;
    ears.left[0] = ears.right[0] = 1.f;
    return ears;
}

void EarFilter::setTarget(const Ears &ears)
{
    Ears clamped = ears;
    clamped.taps = std::clamp(ears.taps, 1, kMaxTaps);
    clamped.delay = std::clamp(ears.delay, -float(kMaxDelay), float(kMaxDelay));
    if (sameEars(clamped, target_))
    {
        return;
    }
    startGlideFromCurrentTaps();
    target_ = clamped;
}

void EarFilter::jumpTo(const Ears &ears)
{
    setTarget(ears);
    fromWeight_ = 0.f;
    delay_ = target_.delay;
}

// from_ becomes the taps as they are now (the taps past each ears' count are 0, so the longer count holds them all)
void EarFilter::startGlideFromCurrentTaps()
{
    if (fromWeight_ <= 0.f)
    {
        from_ = target_;
    }
    else
    {
        from_.taps = std::max(from_.taps, target_.taps);
        for (size_t k = 0; k < size_t(from_.taps); ++k)
        {
            from_.left[k] = fromWeight_ * from_.left[k] + (1.f - fromWeight_) * target_.left[k];
            from_.right[k] = fromWeight_ * from_.right[k] + (1.f - fromWeight_) * target_.right[k];
        }
    }
    fromWeight_ = 1.f;
}

void EarFilter::glide()
{
    fromWeight_ *= 1.f - kGlide;
    fromWeight_ = fromWeight_ < kSettledWeight ? 0.f : fromWeight_;
    delay_ += kGlide * (target_.delay - delay_);
}

// the ear's input `delay` samples late (linear between two samples)
float EarFilter::delayed(int ear, float sample, float delay)
{
    std::array<float, kDelayLine> &line = delayLines_[size_t(ear)];
    line[size_t(delayAt_)] = sample;
    const float late = std::max(0.f, delay);
    const int whole = int(late);
    const float part = late - float(whole);
    const float onTime = line[size_t((delayAt_ - whole + kDelayLine) % kDelayLine)];
    const float earlier = line[size_t((delayAt_ - whole - 1 + kDelayLine) % kDelayLine)];
    return onTime + (earlier - onTime) * part;
}

// the ear's taps over the block's delayed sound: the target's, and while gliding the old taps' share
void EarFilter::filterEar(int ear, const BlockWeights &weights, float *out)
{
    const float *input = history_[size_t(ear)].data() + kMaxTaps;
    std::array<float, kBlockFrames> onTarget, onFrom;
    fir::convolve({tapsOf(target_, ear), target_.taps, input, weights.frames}, onTarget.data());
    if (weights.from[0] <= 0.f)
    {
        for (int f = 0; f < weights.frames; ++f)
        {
            out[f * 2 + ear] = onTarget[size_t(f)];
        }
        return;
    }
    fir::convolve({tapsOf(from_, ear), from_.taps, input, weights.frames}, onFrom.data());
    for (int f = 0; f < weights.frames; ++f)
    {
        out[f * 2 + ear] = onTarget[size_t(f)] + weights.from[size_t(f)] * (onFrom[size_t(f)] - onTarget[size_t(f)]);
    }
}

void EarFilter::processBlock(const float *in, float *out, int frames)
{
    BlockWeights weights;
    weights.frames = frames;
    for (int f = 0; f < frames; ++f)
    {
        glide();
        weights.from[size_t(f)] = fromWeight_;
        history_[0][size_t(kMaxTaps + f)] = delayed(0, in[f * 2], delay_);
        history_[1][size_t(kMaxTaps + f)] = delayed(1, in[f * 2 + 1], -delay_);
        delayAt_ = (delayAt_ + 1) % kDelayLine;
    }
    filterEar(0, weights, out);
    filterEar(1, weights, out);
    for (std::array<float, kMaxTaps + kBlockFrames> &history : history_)
    {
        std::memmove(history.data(), history.data() + frames, kMaxTaps * sizeof(float));
    }
}

void EarFilter::process(const float *in, float *out, size_t frames)
{
    static_assert(kBlockFrames == fir::kBlockFrames);
    for (size_t done = 0; done < frames;)
    {
        const int block = int(std::min<size_t>(kBlockFrames, frames - done));
        processBlock(in + done * 2, out + done * 2, block);
        done += size_t(block);
    }
}

}  // namespace argon::hrtf
