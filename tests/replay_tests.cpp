// Argon: tests for the replay reader (logic/replay.hpp) on the game's own replays, game_data\replays (the
// attract demo's three races). No window or GPU: run with test.bat / test.sh (or ctest in build). Argument: the
// replays folder; without game_data the file tests are skipped.
#include "logic/replay.hpp"
#include "check.hpp"
#include "logic/replay_weapons.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <stdexcept>

namespace
{
using argon::test::checks;
using argon::test::failures;

using namespace argon::replay;

bool near(float a, float b, float tolerance = 1e-3f)
{
    return std::abs(a - b) < tolerance;
}

// an index file's bytes: {time, offset} per entry
std::vector<uint8_t> indexBytes(const std::vector<IndexEntry> &entries)
{
    std::vector<uint8_t> out(entries.size() * 8);
    for (size_t i = 0; i < entries.size(); ++i)
    {
        std::memcpy(out.data() + 8 * i, &entries[i].time, 4);
        std::memcpy(out.data() + 8 * i + 4, &entries[i].offset, 4);
    }
    return out;
}

void nameHashMatchesTheGamesCraftIds()
{
    CHECK(nameHash("craft0") == 0x66617226u);   // the bytes "&raf" of replay1's first entry
    CHECK(nameHash("craft2") == 0x66617224u);
}

void infoReadsTrackAndPilots()
{
    const Info lineup = parseInfo("track flames\r\ncraft0 darius\r\ncraft1 zafir\r\n \r\n");
    CHECK(lineup.track == 3);
    CHECK(lineup.pilots[0] == 3);
    CHECK(lineup.pilots[1] == 11);
    CHECK(lineup.craftCount() == 2);
}

void infoNamesCoverTheAttractTables()
{
    CHECK(trackIdNamed("alt_mars") == 0x11);
    CHECK(pilotIdNamed("kutoo") == 15);
    CHECK(trackIdNamed("nowhere") == -1);
}

void indexStopsAtTheEndMarker()
{
    const std::vector<IndexEntry> index = parseIndex(indexBytes({{0.f, 0}, {0.5f, 0x220}, {-1.f, 0}, {9.f, 0}}));
    CHECK(index.size() == 2);
    CHECK(index[1].offset == 0x220);
}

std::filesystem::path findReplays(int argc, char **argv)
{
    if (argc > 1 && std::filesystem::exists(argv[1]))
    {
        return argv[1];
    }
    for (std::filesystem::path dir = QR_SOURCE_DIR; dir.has_relative_path(); dir = dir.parent_path())
    {
        if (std::filesystem::exists(dir / "game_data" / "replays"))
        {
            return dir / "game_data" / "replays";
        }
    }
    return {};
}

void replay1HeaderMatchesTheGame(const Recording &r)
{
    CHECK(r.lineup.track == 5);         // bracken
    CHECK(r.lineup.pilots[0] == 5);     // marcus
    CHECK(r.lineup.pilots[5] == 11);    // zafir
    CHECK(r.index.size() == 10736);
    CHECK(near(r.endTime(), 214.74f, 0.01f));
}

void replay1FirstCraftEventIsOnTheGrid(const Recording &r)
{
    const CraftEvent &e = r.crafts[0].front();
    CHECK(e.time == 0.f);
    CHECK(near(e.pos.x, 16.01f, 0.01f) && near(e.pos.z, 37.32f, 0.01f));
    CHECK(near(e.rotation[1].y, 1.f));                  // up
    CHECK(near(e.rotation[2].z, -1.f, 0.01f));          // forward -z, the grid's direction
    CHECK(e.controls == ControlFire);
}

void replay1HasEveryCraftAndTheWeapons(const Recording &r)
{
    CHECK(r.crafts[0].size() == 10471);
    CHECK(r.crafts[5].size() == 10387);
    CHECK(r.crafts[6].empty());
    CHECK(r.weapons.size() == 3947);
}

// replay1's first impact: craft 2's projectile 0 of slot 0 hits a craft (kind 1) at 7.42 s
void replay1FirstImpactIsACraftHit(const Recording &r)
{
    auto impact = std::find_if(r.weapons.begin(), r.weapons.end(), isImpact);
    CHECK(impact != r.weapons.end() && impactKindOf(*impact) == 1 && (projectileOf(*impact) == ProjectileId{2, 0, 0}));
    CHECK(impact != r.weapons.end() && glm::length(impactPositionOf(*impact) - glm::vec3(98.16f, -20.40f, -390.93f)) < 0.01f);
}

bool isProjectileState(const WeaponEvent &e)
{
    return e.kind == WeaponEvent::KindProjectile;
}

// a projectile state's direction bytes are its velocity's direction (negated in the file)
void replay1ProjectilesPointAlongTheirVelocity(const Recording &r)
{
    auto state = std::find_if(r.weapons.begin(), r.weapons.end(), isProjectileState);
    CHECK(state != r.weapons.end() && glm::dot(stateForwardOf(*state), glm::normalize(stateVelocityOf(*state))) > 0.99f);
}

void poseFollowsTheEvents(const Recording &r)
{
    const std::vector<CraftEvent> &c = r.crafts[0];
    Pose p;
    CHECK(!poseAt(c, -1.f, p));
    CHECK(poseAt(c, c[100].time, p) && glm::length(p.pos - c[100].pos) < 1e-3f);
    const float mid = 0.5f * (c[100].time + c[101].time);
    CHECK(poseAt(c, mid, p) && glm::length(p.pos - 0.5f * (c[100].pos + c[101].pos)) < 1e-3f);
}

void theOtherReplaysParse(const std::filesystem::path &dir)
{
    const Recording r2 = loadRecording(dir, "replay2");
    CHECK(r2.lineup.track == 0);   // silver
    CHECK(near(r2.endTime(), 223.13f, 0.01f));
    const Recording r3 = loadRecording(dir, "replay3");
    CHECK(r3.lineup.track == 3);   // flames
    CHECK(near(r3.endTime(), 218.06f, 0.01f));
}

void realFiles(const std::filesystem::path &dir)
{
    const Recording r1 = loadRecording(dir, "replay1");
    replay1HeaderMatchesTheGame(r1);
    replay1FirstCraftEventIsOnTheGrid(r1);
    replay1HasEveryCraftAndTheWeapons(r1);
    replay1FirstImpactIsACraftHit(r1);
    replay1ProjectilesPointAlongTheirVelocity(r1);
    poseFollowsTheEvents(r1);
    theOtherReplaysParse(dir);
}

}  // namespace

int main(int argc, char **argv)
{
    nameHashMatchesTheGamesCraftIds();
    infoReadsTrackAndPilots();
    infoNamesCoverTheAttractTables();
    indexStopsAtTheEndMarker();
    const std::filesystem::path dir = findReplays(argc, argv);
    if (dir.empty())
    {
        std::printf("game_data\\replays not found: the file tests are skipped\n");
    }
    else
    {
        try
        {
            realFiles(dir);
        }
        catch (const std::exception &e)
        {
            ++failures;
            std::printf("reading %s: %s\n", dir.string().c_str(), e.what());
        }
    }
    std::printf("%d checks, %d failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
