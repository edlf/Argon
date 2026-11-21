// Argon: tests for the tracks' visibility sectors (track_visibility.hpp: visdata.vis, 0xafcf0) on a built
// file and on the game's own (skipped without game_data). Run with test.bat / test.sh.
#include "model/cmm.hpp"
#include "check.hpp"
#include "track/track_visibility.hpp"

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>

using namespace argon;

namespace
{
using argon::test::checks;
using argon::test::failures;

struct SectorSpec
{
    glm::vec3 position;
    std::vector<uint32_t> meshes, neighbours;
};

void appendWord(std::vector<uint8_t> &bytes, uint32_t value)
{
    const size_t at = bytes.size();
    bytes.resize(at + 4);
    std::memcpy(bytes.data() + at, &value, 4);
}

void appendFloat(std::vector<uint8_t> &bytes, float value)
{
    uint32_t word;
    std::memcpy(&word, &value, 4);
    appendWord(bytes, word);
}

// header (count, stale pointer, 16 unused bytes), the 0x1c-byte sectors, then each sector's meshes and neighbours
std::vector<uint8_t> visFile(const std::vector<SectorSpec> &sectors)
{
    std::vector<uint8_t> bytes;
    appendWord(bytes, uint32_t(sectors.size()));
    bytes.resize(0x18, 0xcd);
    for (const SectorSpec &s : sectors)
    {
        appendFloat(bytes, s.position.x);
        appendFloat(bytes, s.position.y);
        appendFloat(bytes, s.position.z);
        appendWord(bytes, uint32_t(s.neighbours.size()));
        appendWord(bytes, uint32_t(s.meshes.size()));
        appendWord(bytes, 0xdeadbeef);
        appendWord(bytes, 0xdeadbeef);
    }
    for (const SectorSpec &s : sectors)
    {
        for (uint32_t mesh : s.meshes)
        {
            appendWord(bytes, mesh);
        }
        for (uint32_t neighbour : s.neighbours)
        {
            appendWord(bytes, neighbour);
        }
    }
    return bytes;
}

// three sectors in a row along x; the middle one sees both ends, the ends only the middle
TrackVisibility threeSectors()
{
    TrackVisibility vis;
    vis.read(visFile({{{0, 0, 0}, {0, 1}, {1}}, {{100, 0, 0}, {2}, {0, 2}}, {{200, 0, 0}, {3}, {1}}}));
    return vis;
}

void readsTheSectorsAndTheirLists()
{
    const TrackVisibility vis = threeSectors();
    CHECK(vis.sectors().size() == 3 && vis.sectors()[1].meshes == std::vector<uint32_t>{2} && vis.sectors()[1].neighbours == std::vector<uint32_t>({0, 2}));
}

void nearestSectorIsByDistance()
{
    CHECK(threeSectors().nearestSector({180, 30, 0}) == 2);
}

void eyeSeesItsSectorAndTheNeighboursMeshes()
{
    const std::vector<bool> visible = threeSectors().visibleMeshes({10, 0, 0}, 4);
    CHECK(visible == std::vector<bool>({true, true, true, false}));
}

void neighboursOfNeighboursStayHidden()
{   // from the last sector: its mesh 3 and the middle's mesh 2, not the first's 0 / 1
    const std::vector<bool> visible = threeSectors().visibleMeshes({190, 0, 0}, 4);
    CHECK(visible == std::vector<bool>({false, false, true, true}));
}

void noSectorsShowEverything()
{
    CHECK(TrackVisibility().visibleMeshes({0, 0, 0}, 3) == std::vector<bool>(3, true));
}

void truncatedFileIsRejected()
{
    std::vector<uint8_t> bytes = visFile({{{0, 0, 0}, {0, 1, 2}, {}}});
    bytes.resize(bytes.size() - 4);
    TrackVisibility vis;
    CHECK(!vis.read(bytes) && vis.empty());
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

bool indicesFitTheModel(const TrackVisibility &vis, const Cmm &model)
{
    for (const VisSector &s : vis.sectors())
    {
        for (uint32_t mesh : s.meshes)
        {
            if (mesh >= model.meshes.size())
            {
                return false;
            }
        }
        for (uint32_t neighbour : s.neighbours)
        {
            if (neighbour >= vis.sectors().size())
            {
                return false;
            }
        }
    }
    return true;
}

void gameFilesFitTheirTracks()
{
    const std::filesystem::path tracks = findTracks();
    if (tracks.empty())
    {
        std::printf("game_data\\data\\tracks not found: the file tests are skipped\n");
        return;
    }
    int files = 0;
    for (const auto &entry : std::filesystem::directory_iterator(tracks))
    {
        const std::filesystem::path visPath = entry.path() / "visdata.vis";
        if (!std::filesystem::exists(visPath))
        {
            continue;
        }
        std::vector<uint8_t> bytes;
        Cmm model;
        TrackVisibility vis;
        CHECK(readFile(visPath.string(), bytes) && vis.read(bytes) && !vis.empty());
        model.load((entry.path() / "model.cmm").string());
        CHECK(indicesFitTheModel(vis, model));
        ++files;
    }
    CHECK(files == 16);
}

} // namespace

int main()
{
    readsTheSectorsAndTheirLists();
    nearestSectorIsByDistance();
    eyeSeesItsSectorAndTheNeighboursMeshes();
    neighboursOfNeighboursStayHidden();
    noSectorsShowEverything();
    truncatedFileIsRejected();
    gameFilesFitTheirTracks();
    std::printf("track_visibility_tests: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
