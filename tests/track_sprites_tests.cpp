// Argon: tests for sprite.dat's quads (track_sprites.hpp: 0xbdf40, 0xbd8f0, 0xbcfa0, 0xbd1b0) on built records
// and on the game's own files (skipped without game_data). Run with test.bat / test.sh.
#include "original/track_records.hpp"
#include "check.hpp"
#include "track/track_sprites.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

using namespace argon;

namespace
{
using argon::test::checks;
using argon::test::failures;

bool near(const glm::vec3 &a, const glm::vec3 &b)
{
    return glm::length(a - b) < 1e-4f;
}

bool near(const glm::vec2 &a, const glm::vec2 &b)
{
    return glm::length(a - b) < 1e-5f;
}

// a record whose corner k sits at (k, 10 k, 100 k) with uv floats 0.1 .. 0.8 and colours 0x11..0x44
original::QuadSprite numberedRecord(int16_t frame, uint32_t flags, const glm::vec3 &pos)
{
    original::QuadSprite r{};
    r.frame = frame;
    r.flags = flags;
    r.position[0] = pos.x;
    r.position[1] = pos.y;
    r.position[2] = pos.z;
    for (int k = 0; k < 4; ++k)
    {
        r.cornerOffsets[k][0] = float(k);
        r.cornerOffsets[k][1] = float(10 * k);
        r.cornerOffsets[k][2] = float(100 * k);
        r.cornerColours[k] = 0x11111111u * uint32_t(k + 1);
    }
    for (int k = 0; k < 8; ++k)
    {
        r.cornerUvs[k] = 0.1f * float(k + 1);
    }
    return r;
}

std::vector<uint8_t> spriteFile(const std::vector<original::QuadSprite> &records)
{
    original::QuadSpriteHeader header{};
    header.count = uint32_t(records.size());
    std::vector<uint8_t> bytes(sizeof header + records.size() * sizeof(original::QuadSprite));
    std::memcpy(bytes.data(), &header, sizeof header);
    if (!records.empty())
    {
        std::memcpy(bytes.data() + sizeof header, records.data(), records.size() * sizeof(original::QuadSprite));
    }
    return bytes;
}

TrackSprite spriteAt(const glm::vec3 &pos, bool additive)
{
    TrackSprite s;
    s.pos = pos;
    s.additive = additive;
    return s;
}

void cornersComeFromTheRecordsSlots()
{
    const auto sprites = readTrackSprites(spriteFile({numberedRecord(2, 0, {1, 2, 3})}));
    CHECK(sprites.size() == 1);
    const TrackSprite &s = sprites[0];
    CHECK(near(s.pos, {1, 2, 3}) && s.facing == SpriteFacing::Camera && !s.additive);
    CHECK(near(s.corners[0].offset, {2, 20, 200}) && near(s.corners[1].offset, {3, 30, 300}));
    CHECK(near(s.corners[2].offset, {1, 10, 100}) && near(s.corners[3].offset, {0, 0, 0}));
    CHECK(s.corners[0].argb == 0x11111111u && s.corners[3].argb == 0x44444444u);
}

void uvsTakeTheAtlasLowerHalf()
{
    const auto s = readTrackSprites(spriteFile({numberedRecord(1, 0, {})}))[0];
    CHECK(near(s.corners[0].uv, {0.1f, 0.8f * 0.5f + 0.5f}) && near(s.corners[1].uv, {0.3f, 0.6f * 0.5f + 0.5f}));
    CHECK(near(s.corners[2].uv, {0.7f, 0.2f * 0.5f + 0.5f}) && near(s.corners[3].uv, {0.5f, 0.4f * 0.5f + 0.5f}));
}

void flagBitZeroIsAdditive()
{
    const auto sprites = readTrackSprites(spriteFile({numberedRecord(1, 3, {}), numberedRecord(1, 2, {})}));
    CHECK(sprites.size() == 2 && sprites[0].additive && !sprites[1].additive);
}

void unknownFrameFacesTheWorld()
{
    CHECK(readTrackSprites(spriteFile({numberedRecord(7, 0, {})}))[0].facing == SpriteFacing::World);
}

void shortFileKeepsWholeRecords()
{
    std::vector<uint8_t> bytes = spriteFile({numberedRecord(1, 0, {}), numberedRecord(1, 0, {})});
    bytes.resize(bytes.size() - 1);
    CHECK(readTrackSprites(bytes).size() == 1);
    CHECK(readTrackSprites({}).empty());
}

void cornerIsPositionPlusTurnedOffset()
{
    TrackSprite s = spriteAt({10, 20, 30}, false);
    s.corners[1].offset = {1, 2, 3};
    SpriteBasis b;
    b.x = {0, 0, 1};
    b.y = {1, 0, 0};
    b.z = {0, 1, 0};
    CHECK(near(spriteCornerPosition(s, 1, b), {12, 23, 31}));
}

void blendedListIsFarToNear()
{
    const std::vector<TrackSprite> sprites = {
        spriteAt({10, 0, 0}, false), spriteAt({0, 0, 900}, false), spriteAt({0, -50, 0}, false), spriteAt({0, 0, 2000}, false), spriteAt({0, 0, 400}, true)};
    CHECK((blendedSpriteOrder(sprites, glm::vec3(0)) == std::vector<int>{1, 2, 0}));
}

void blendedBucketHoldsTheLastAddedFirst()
{
    const std::vector<TrackSprite> sprites = {spriteAt({100, 0, 0}, false), spriteAt({0, 100.2f, 0}, false)};
    CHECK((blendedSpriteOrder(sprites, glm::vec3(0)) == std::vector<int>{1, 0}));
}

void blendedListSkipsTheNearestBucket()
{
    CHECK(blendedSpriteOrder({spriteAt({0.4f, 0, 0}, false)}, glm::vec3(0)).empty());
}

// the blended list measures the largest axis distance, the additive one the straight distance
void cullDistancesDifferPerList()
{
    const glm::vec3 diagonal(1200, 1200, 0);
    CHECK(blendedSpriteOrder({spriteAt(diagonal, false)}, glm::vec3(0)).size() == 1);
    CHECK(additiveSpriteOrder({spriteAt(diagonal, true)}, glm::vec3(0)).empty());
}

void additiveListKeepsPoolOrder()
{
    const std::vector<TrackSprite> sprites = {spriteAt({0, 0, 900}, true), spriteAt({0, 0, 10}, false), spriteAt({0, 0, 100}, true)};
    CHECK((additiveSpriteOrder(sprites, glm::vec3(0)) == std::vector<int>{0, 2}));
}

std::filesystem::path findTracks()
{
    for (std::filesystem::path dir = QR_SOURCE_DIR; dir.has_relative_path(); dir = dir.parent_path())
    {
        if (std::filesystem::exists(dir / "game_data" / "data" / "tracks"))
        {
            return dir / "game_data" / "data" / "tracks";
        }
    }
    return {};
}

size_t spritesOf(const std::filesystem::path &tracks, const std::string &track)
{
    std::ifstream f(tracks / track / "sprite.dat", std::ios::binary);
    const std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    return readTrackSprites(bytes).size();
}

void gameFilesHoldTheirCounts()
{
    const std::filesystem::path tracks = findTracks();
    if (tracks.empty())
    {
        std::printf("game_data\\data\\tracks not found: the file tests are skipped\n");
        return;
    }
    CHECK(spritesOf(tracks, "bracken") == 32 && spritesOf(tracks, "alt_bracken") == 98);
    CHECK(spritesOf(tracks, "mount") == 348 && spritesOf(tracks, "alt_mount") == 439);
    CHECK(spritesOf(tracks, "silver") == 9 && spritesOf(tracks, "kalau") == 0);
}

} // namespace

int main()
{
    cornersComeFromTheRecordsSlots();
    uvsTakeTheAtlasLowerHalf();
    flagBitZeroIsAdditive();
    unknownFrameFacesTheWorld();
    shortFileKeepsWholeRecords();
    cornerIsPositionPlusTurnedOffset();
    blendedListIsFarToNear();
    blendedBucketHoldsTheLastAddedFirst();
    blendedListSkipsTheNearestBucket();
    cullDistancesDifferPerList();
    additiveListKeepsPoolOrder();
    gameFilesHoldTheirCounts();
    std::printf("track_sprites_tests: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
