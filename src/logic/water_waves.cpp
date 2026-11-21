// The water's wave heightfield for the physics (water_waves.hpp): 0xcd4f0 (frames) and 0xce6b0 (depth query).
#include "logic/water_waves.hpp"

#include <algorithm>
#include <cmath>

namespace argon
{

namespace
{

constexpr double kDegrees = 3.14159265358979323846 / 180.0;
constexpr int kCellsPerTile = 16;
constexpr int kGameFramesPerWaveFrame = 8;
constexpr uint32_t kPhysicsOnlyBox = 1;          // WaterBox::a bit 0: no waves (and no drawn surface)

// 0xcd4f0: x87 sin / cos, stored with fistp (round to nearest, ties to even)
int16_t waveShort(int frame, int i, int j)
{
    const double phase = 12.0 * frame;
    const double h = 5000.0 * std::sin((45.0 * i + phase) * kDegrees) + 5000.0 * std::cos((45.0 * j + phase) * kDegrees) +
                     2000.0 * std::sin((phase - 90.0 * i) * kDegrees) + 2000.0 * std::cos((phase - 90.0 * j) * kDegrees);
    return int16_t(std::nearbyint(h));
}

struct FrameBlend
{
    int first, second;   // wave frames
    float weight;        // of the second
};

// 0xce6b0's frame: the game frame round(seconds x 60), a wave frame every 8 of them, blended in eighths
FrameBlend frameBlendAt(float seconds)
{
    const int gameFrame = int(std::nearbyint(seconds * 60.f));
    const int first = (gameFrame / kGameFramesPerWaveFrame) % WaterWaves::kFrames;
    const int second = first + 1 < WaterWaves::kFrames ? first + 1 : 0;
    return {first, second, float(gameFrame % kGameFramesPerWaveFrame) * 0.125f};
}

struct CellPoint
{
    int i, j;            // the cell's first vertex in the tile
    float alongX, alongZ;   // 0..1 across the cell
};

CellPoint cellPointAt(float x, float z)
{
    const float u = (x - std::floor(x / WaterWaves::kTileSize) * WaterWaves::kTileSize) / WaterWaves::kGridStep;
    const float v = (z - std::floor(z / WaterWaves::kTileSize) * WaterWaves::kTileSize) / WaterWaves::kGridStep;
    const float i = std::floor(u), j = std::floor(v);
    return {int(i) % kCellsPerTile, int(j) % kCellsPerTile, std::clamp(u - i, 0.f, 1.f), std::clamp(v - j, 0.f, 1.f)};
}

// a grid vertex's height between the two wave frames
float blendedHeight(const WaterWaves &waves, const FrameBlend &time, int i, int j)
{
    return float(waves.height(time.first, i, j)) * (1.f - time.weight) + float(waves.height(time.second, i, j)) * time.weight;
}

bool holdsXz(const WaterBox &box, const glm::vec3 &point)
{
    return point.x >= box.min[0] && point.x < box.max[0] && point.z >= box.min[2] && point.z < box.max[2];
}

} // namespace

WaterWaves::WaterWaves(float waveScale) : scale_(waveScale), heights_(size_t(kFrames) * kVertices * kVertices)
{
    for (int frame = 0; frame < kFrames; ++frame)
    {
        for (int j = 0; j < kVertices; ++j)
        {
            for (int i = 0; i < kVertices; ++i)
            {
                heights_[(size_t(frame) * kVertices + j) * kVertices + i] = waveShort(frame, i, j);
            }
        }
    }
}

int16_t WaterWaves::height(int frame, int i, int j) const
{
    return heights_[(size_t(frame) * kVertices + j) * kVertices + i];
}

float WaterWaves::offsetAt(float x, float z, float seconds) const
{
    const FrameBlend time = frameBlendAt(seconds);
    const CellPoint cell = cellPointAt(x, z);
    const float nearRow = blendedHeight(*this, time, cell.i + 1, cell.j) * cell.alongX + blendedHeight(*this, time, cell.i, cell.j) * (1.f - cell.alongX);
    const float farRow =
        blendedHeight(*this, time, cell.i + 1, cell.j + 1) * cell.alongX + blendedHeight(*this, time, cell.i, cell.j + 1) * (1.f - cell.alongX);
    return (nearRow * (1.f - cell.alongZ) + farRow * cell.alongZ) * kShortToUnit * scale_;
}

bool waterDepthAt(const std::vector<WaterBox> &boxes, const WaterWaves &waves, const glm::vec3 &point, float seconds, float &depth)
{
    for (const WaterBox &box : boxes)
    {
        if (!holdsXz(box, point) || point.y < box.min[1])
        {
            continue;
        }
        const float wave = (box.flags & kPhysicsOnlyBox) ? 0.f : waves.offsetAt(point.x, point.z, seconds);
        depth = point.y - (box.max[1] + wave);
        return true;
    }
    return false;
}

} // namespace argon
