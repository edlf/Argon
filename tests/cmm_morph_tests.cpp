// Argon: tests for the morph meshes (cmm_morph.hpp: the "Keyframe ..." effects' blend, 0x7b7e0) and their
// streams in objects\explodefirelarge (skipped without game_data). Run with test.bat / test.sh.
#include "model/cmm_morph.hpp"
#include "check.hpp"

#include <cmath>
#include <cstdio>
#include <filesystem>

using namespace argon;

namespace
{
using argon::test::checks;
using argon::test::failures;

bool near(float a, float b)
{
    return std::abs(a - b) < 1e-4f;
}

// explodefirelarge's streams: 10 keyframes, 3 a second (a 3.33 s cycle)
CmmMorph tenKeysAtThreeASecond()
{
    CmmMorph morph;
    morph.kind = 1;
    morph.streams = 10;
    morph.streamsPerSecond = 3.f;
    morph.stride = 12;
    return morph;
}

float secondsAtFrame(float frame)
{
    return frame / 3.f;
}

void weightIsTheDistancePastTheNearestKey()
{
    const MorphBlend blend = morphBlendAt(secondsAtFrame(1.2f), tenKeysAtThreeASecond());
    CHECK(blend.from == 1 && blend.to == 2 && near(blend.weight, 0.2f));
}

void frameRoundsToTheNearestKey()
{   // fistp rounds: frame 1.7 is key 2 with weight -0.3, leaning away from key 3
    const MorphBlend blend = morphBlendAt(secondsAtFrame(1.7f), tenKeysAtThreeASecond());
    CHECK(blend.from == 2 && blend.to == 3 && near(blend.weight, -0.3f));
}

void lastKeyBlendsIntoTheFirst()
{
    const MorphBlend blend = morphBlendAt(secondsAtFrame(9.2f), tenKeysAtThreeASecond());
    CHECK(blend.from == 9 && blend.to == 0);
}

void frameRoundedToTheKeyCountRestartsWithFullWeight()
{   // key 10 does not exist: key 0, weight frame - 0 clamped to 1
    const MorphBlend blend = morphBlendAt(secondsAtFrame(9.7f), tenKeysAtThreeASecond());
    CHECK(blend.from == 0 && blend.to == 1 && near(blend.weight, 1.f));
}

void cycleLoops()
{
    const MorphBlend first = morphBlendAt(secondsAtFrame(4.1f), tenKeysAtThreeASecond());
    const MorphBlend later = morphBlendAt(secondsAtFrame(4.1f) + 10.f / 3.f, tenKeysAtThreeASecond());
    CHECK(first.from == later.from && near(first.weight, later.weight));
}

void meshWithoutStreamsStaysOnKeyZero()
{
    const MorphBlend blend = morphBlendAt(1.f, CmmMorph{});
    CHECK(blend.from == 0 && blend.to == 0 && near(blend.weight, 0.f));
}

std::filesystem::path findExplodeFireLarge()
{
    for (std::filesystem::path dir = QR_SOURCE_DIR; dir.has_relative_path(); dir = dir.parent_path())
    {
        const std::filesystem::path model = dir / "game_data" / "data" / "objects" / "explodefirelarge" / "model.cmm";
        if (std::filesystem::exists(model))
        {
            return model;
        }
    }
    return {};
}

void explodeFireLargeHasTenPositionKeysPerMesh(const Cmm &model)
{
    int morphMeshes = 0;
    for (const CmmMesh &m : model.meshes)
    {
        morphMeshes += m.morph.kind == 1 && m.morph.streams == 10 && near(m.morph.streamsPerSecond, 3.f);
    }
    CHECK(morphMeshes == 18);
}

void morphMovesTheVertices(const Cmm &model)
{
    const CmmMesh &mesh = model.meshes[0];
    std::vector<Vertex> start(mesh.numVerts), later(mesh.numVerts);
    model.morph(mesh, 0.f, start.data());
    model.morph(mesh, 1.f, later.data());
    float moved = 0.f;
    for (uint32_t v = 0; v < mesh.numVerts; ++v)
    {
        moved = std::fmax(moved, std::abs(start[v].pos[1] - later[v].pos[1]));
    }
    CHECK(moved > 0.f);
}

void explodeFireLargeTests()
{
    const std::filesystem::path path = findExplodeFireLarge();
    if (path.empty())
    {
        std::printf("game_data\\data\\objects\\explodefirelarge not found: the file tests are skipped\n");
        return;
    }
    Cmm model;
    model.load(path.string());
    if (model.meshes.empty())
    {
        return;
    }
    explodeFireLargeHasTenPositionKeysPerMesh(model);
    morphMovesTheVertices(model);
}

} // namespace

int main()
{
    weightIsTheDistancePastTheNearestKey();
    frameRoundsToTheNearestKey();
    lastKeyBlendsIntoTheFirst();
    frameRoundedToTheKeyCountRestartsWithFullWeight();
    cycleLoops();
    meshWithoutStreamsStaysOnKeyZero();
    explodeFireLargeTests();
    std::printf("cmm_morph_tests: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
