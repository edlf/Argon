// Argon: animation track blocks (cmm_tracks.hpp)
#include "model/cmm_tracks.hpp"

#include <cmath>
#include <cstring>

#include "original/cmm_records.hpp"
#include "original/record_read.hpp"

namespace argon
{

namespace
{

using original::readAt;

constexpr int kMaxTracks = 4096;
constexpr uint32_t kMaxKeys = 100000;
constexpr float kDegToRad = 3.14159265358979f / 180.f;
constexpr uint32_t kPositionKeys = 1, kScaleKeys = 2, kRotationKeys = 4;

// Euler degrees -> X x Y x Z (D3D row-vector, 0xf2ec4 / 0xf2f3e / 0xf2fb9) -> quaternion x, y, z, w (0xf33fe)
void eulerDegreesToQuaternion(float ax, float ay, float az, float *quaternion)
{
    const float cx = std::cos(ax * kDegToRad), sx = std::sin(ax * kDegToRad);
    const float cy = std::cos(ay * kDegToRad), sy = std::sin(ay * kDegToRad);
    const float cz = std::cos(az * kDegToRad), sz = std::sin(az * kDegToRad);
    // Rx = [1 0 0; 0 c s; 0 -s c], Ry = [c 0 -s; 0 1 0; s 0 c], Rz = [c s 0; -s c 0; 0 0 1]
    const float rotation[3][3] = {
        {cy * cz, cy * sz, -sy}, {sx * sy * cz - cx * sz, sx * sy * sz + cx * cz, sx * cy}, {cx * sy * cz + sx * sz, cx * sy * sz - sx * cz, cx * cy}};
    const float trace = rotation[0][0] + rotation[1][1] + rotation[2][2];
    if (trace > 0.f)
    {
        const float w = std::sqrt(trace + 1.f) * 0.5f, f = 0.25f / w;
        quaternion[0] = (rotation[1][2] - rotation[2][1]) * f;
        quaternion[1] = (rotation[2][0] - rotation[0][2]) * f;
        quaternion[2] = (rotation[0][1] - rotation[1][0]) * f;
        quaternion[3] = w;
        return;
    }
    int i0 = (rotation[1][1] > rotation[0][0]) ? 1 : 0;
    if (rotation[2][2] > rotation[i0][i0])
    {
        i0 = 2;
    }
    const int i1 = (i0 + 1) % 3, i2 = (i0 + 2) % 3;
    const float v = std::sqrt(rotation[i0][i0] - rotation[i1][i1] - rotation[i2][i2] + 1.f) * 0.5f, f = 0.25f / v;
    quaternion[i0] = v;
    quaternion[i1] = (rotation[i0][i1] + rotation[i1][i0]) * f;
    quaternion[i2] = (rotation[i0][i2] + rotation[i2][i0]) * f;
    quaternion[3] = (rotation[i1][i2] - rotation[i2][i1]) * f;
}

size_t copyFloats(const std::vector<uint8_t> &data, size_t at, size_t count, std::vector<float> &out)
{
    out.resize(count);
    std::memcpy(out.data(), data.data() + at, count * 4);
    return at + count * 4;
}

size_t readRotationKeys(const std::vector<uint8_t> &data, size_t at, CmmTrack &track)
{
    track.quat.resize(size_t(track.keys) * 4);
    for (uint32_t k = 0; k < track.keys; ++k)
    {
        const size_t key = at + k * 16;
        eulerDegreesToQuaternion(readAt<float>(data, key), readAt<float>(data, key + 4), readAt<float>(data, key + 8), &track.quat[size_t(k) * 4]);
    }
    return at + size_t(track.keys) * 16;
}

size_t keyBytes(const CmmTrack &track)
{
    const size_t perKey = ((track.flags & kPositionKeys) ? 12 : 0) + ((track.flags & kRotationKeys) ? 16 : 0) + ((track.flags & kScaleKeys) ? 12 : 0);
    return size_t(track.keys) * perKey;
}

// per track: positions, rotations, scales
std::optional<size_t> readTrack(const std::vector<uint8_t> &data, const original::CmmAnimationTrack &record, size_t keys, CmmTrack &track)
{
    track.flags = record.keyFlags;
    track.keys = record.keyCount;
    track.fps = record.keysPerSecond;
    track.name = fixedString(record.boneName, sizeof record.boneName);
    if (track.keys == 0 || track.keys > kMaxKeys || keys + keyBytes(track) > data.size())
    {
        return std::nullopt;
    }
    if (track.flags & kPositionKeys)
    {
        keys = copyFloats(data, keys, size_t(track.keys) * 3, track.pos);
    }
    if (track.flags & kRotationKeys)
    {
        keys = readRotationKeys(data, keys, track);
    }
    if (track.flags & kScaleKeys)
    {
        keys = copyFloats(data, keys, size_t(track.keys) * 3, track.scl);
    }
    return keys;
}

} // namespace

std::string fixedString(const char *chars, size_t capacity)
{
    return std::string(chars, strnlen(chars, capacity));
}

std::optional<size_t> readAnimationTracks(const std::vector<uint8_t> &data, AnimationBlock block, std::vector<CmmTrack> &out)
{
    constexpr size_t kTrackSize = sizeof(original::CmmAnimationTrack);
    out.clear();
    if (block.count <= 0 || block.count >= kMaxTracks || block.at + size_t(block.count) * kTrackSize > data.size())
    {
        return std::nullopt;
    }
    std::optional<size_t> keys = block.at + size_t(block.count) * kTrackSize;
    for (int i = 0; i < block.count && keys; ++i)
    {
        const auto record = readAt<original::CmmAnimationTrack>(data, block.at + size_t(i) * kTrackSize);
        out.emplace_back();
        keys = readTrack(data, record, *keys, out.back());
    }
    if (!keys)
    {
        out.clear();
    }
    return keys;
}

} // namespace argon
