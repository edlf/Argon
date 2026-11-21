// Argon: the MIT KEMAR HRTF, see kemar.hpp.
#include "audio/kemar.hpp"

#include <algorithm>
#include <stdexcept>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iterator>
#include <utility>

namespace argon::hrtf
{
namespace
{
constexpr uint32_t kVersion = 1;
constexpr size_t kMagicBytes = 4;

class ByteReader
{
  public:
    explicit ByteReader(std::vector<uint8_t> bytes) : bytes_(std::move(bytes))
    {
    }
    template <class Value>
    bool read(Value &value)
    {
        return readBytes(&value, sizeof value);
    }
    bool readFloats(float *values, size_t count)
    {
        return readBytes(values, count * sizeof(float));
    }
    bool atEnd() const
    {
        return at_ == bytes_.size();
    }

  private:
    bool readBytes(void *to, size_t count)
    {
        if (count > bytes_.size() - at_)
        {
            return false;
        }
        std::memcpy(to, bytes_.data() + at_, count);
        at_ += count;
        return true;
    }

    std::vector<uint8_t> bytes_;
    size_t at_ = 0;
};

std::vector<uint8_t> readFile(const std::filesystem::path &path)
{
    std::ifstream file(path, std::ios::binary);
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

bool readHeader(ByteReader &reader, uint32_t &taps, uint32_t &ringCount)
{
    char magic[kMagicBytes] = {};
    uint32_t version = 0;
    return reader.read(magic) && std::memcmp(magic, "ARKM", kMagicBytes) == 0 && reader.read(version) && version == kVersion && reader.read(taps) &&
           taps > 0 && taps <= uint32_t(kMaxTaps) && reader.read(ringCount);
}

bool readMeasurement(ByteReader &reader, int taps, KemarTables::Measurement &measurement)
{
    int32_t azimuth = 0;
    measurement.ears.taps = taps;
    if (!reader.read(azimuth) || !reader.read(measurement.ears.delay) || !reader.readFloats(measurement.ears.left.data(), size_t(taps)) ||
        !reader.readFloats(measurement.ears.right.data(), size_t(taps)))
    {
        return false;
    }
    measurement.azimuth = azimuth;
    return azimuth >= 0 && azimuth <= 180 && std::isfinite(measurement.ears.delay);
}

bool readRing(ByteReader &reader, int taps, KemarTables::Ring &ring)
{
    int32_t elevation = 0;
    uint32_t count = 0;
    if (!reader.read(elevation) || !reader.read(count) || count == 0)
    {
        return false;
    }
    ring.elevation = elevation;
    ring.measurements.resize(count);
    for (KemarTables::Measurement &measurement : ring.measurements)
    {
        if (!readMeasurement(reader, taps, measurement))
        {
            return false;
        }
    }
    return true;
}

const KemarTables::Ring &nearestRing(const KemarTables &tables, float elevation)
{
    const KemarTables::Ring *nearest = &tables.rings.front();
    for (const KemarTables::Ring &ring : tables.rings)
    {
        if (std::abs(float(ring.elevation) - elevation) < std::abs(float(nearest->elevation) - elevation))
        {
            nearest = &ring;
        }
    }
    return *nearest;
}

const KemarTables::Measurement &nearestMeasurement(const KemarTables::Ring &ring, float azimuth)
{
    const KemarTables::Measurement *nearest = &ring.measurements.front();
    for (const KemarTables::Measurement &measurement : ring.measurements)
    {
        if (std::abs(float(measurement.azimuth) - azimuth) < std::abs(float(nearest->azimuth) - azimuth))
        {
            nearest = &measurement;
        }
    }
    return *nearest;
}
// the first `taps` taps, the last kKemarFadeTaps faded out; ears no longer than that as they are (the converter faded them)
Ears cutTo(const Ears &ears, int taps)
{
    if (taps >= ears.taps)
    {
        return ears;
    }
    constexpr float kQuarterTurn = 1.5707963f;
    Ears cut = ears;
    std::fill(cut.left.begin() + taps, cut.left.begin() + ears.taps, 0.f);
    std::fill(cut.right.begin() + taps, cut.right.begin() + ears.taps, 0.f);
    cut.taps = taps;
    const int fadeFrom = std::max(0, taps - kKemarFadeTaps);
    for (int k = fadeFrom; k < taps; ++k)
    {
        const float fade = std::cos(kQuarterTurn * float(k - fadeFrom + 1) / float(kKemarFadeTaps));
        cut.left[size_t(k)] *= fade * fade;
        cut.right[size_t(k)] *= fade * fade;
    }
    return cut;
}
}  // namespace

KemarTables loadKemar(const std::filesystem::path &file)
{
    ByteReader reader(readFile(file));
    uint32_t taps = 0, ringCount = 0;
    if (!readHeader(reader, taps, ringCount) || ringCount == 0)
    {
        throw std::runtime_error("no MIT KEMAR HRTF in " + file.string() + " (tools/assets/convert_kemar.py)");
    }
    KemarTables tables;
    tables.rings.resize(ringCount);
    for (KemarTables::Ring &ring : tables.rings)
    {
        if (!readRing(reader, int(taps), ring))
        {
            throw std::runtime_error(file.string() + ": a MIT KEMAR ring is cut short or out of range");
        }
    }
    if (!reader.atEnd())
    {
        throw std::runtime_error(file.string() + ": bytes after the MIT KEMAR rings");
    }
    return tables;
}

Ears kemarEarsFor(const KemarTables &tables, const Direction &direction, int taps)
{
    if (tables.empty())
    {
        return passThroughEars();
    }
    Ears ears = cutTo(nearestMeasurement(nearestRing(tables, direction.elevation), std::abs(direction.azimuth)).ears, taps);
    if (direction.azimuth < 0.f)
    {
        std::swap(ears.left, ears.right);
        ears.delay = -ears.delay;
    }
    return ears;
}

}  // namespace argon::hrtf
