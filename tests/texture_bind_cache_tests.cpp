// Argon: tests for the texture-bind cache (render\texture_bind_cache.hpp: d3d_set_texture 0xcbb40 / 0xcbbd2).
// Run with test.bat / test.sh.
#include "render/texture_bind_cache.hpp"
#include "check.hpp"

#include <cstdio>

namespace
{
using argon::test::checks;
using argon::test::failures;

const int kModelA = 1, kModelB = 2;
const void *const modelA = &kModelA;
const void *const modelB = &kModelB;

argon::MeshTextures oneStage(int texture, float rate)
{
    argon::MeshTextures mesh;
    mesh.count = 1;
    mesh.texture[0] = texture;
    mesh.rate[0] = rate;
    return mesh;
}

argon::MeshTextures twoStages(int texture, float rate)
{
    argon::MeshTextures mesh = oneStage(texture, rate);
    mesh.count = 2;
    mesh.texture[1] = texture;
    mesh.rate[1] = rate;
    return mesh;
}

void firstBindKeepsItsRate()
{
    argon::TextureBindCache cache;

    cache.bindMesh(modelA, oneStage(14, 60.f));

    CHECK(cache.bound(0)->rate == 60.f);
}

void sameSlotKeepsTheFirstMeshsRate()
{
    argon::TextureBindCache cache;
    cache.bindMesh(modelA, oneStage(14, 60.f));

    cache.bindMesh(modelA, oneStage(14, 30.f));

    CHECK(cache.bound(0)->rate == 60.f);
}

void otherTextureRebinds()
{
    argon::TextureBindCache cache;
    cache.bindMesh(modelA, oneStage(14, 60.f));

    cache.bindMesh(modelA, oneStage(15, 30.f));

    CHECK(cache.bound(0)->rate == 30.f);
}

void sameTextureOfAnotherModelRebinds()
{
    argon::TextureBindCache cache;
    cache.bindMesh(modelA, oneStage(14, 60.f));

    cache.bindMesh(modelB, oneStage(14, 30.f));

    CHECK(cache.bound(0)->rate == 30.f);
}

void sameSlotOnAnotherStageBindsThatStage()
{
    argon::TextureBindCache cache;
    cache.bindMesh(modelA, oneStage(14, 60.f));

    cache.bind(1, argon::StageBind{argon::TextureSlot{modelA, 14}, 30.f});

    CHECK(cache.bound(1)->rate == 30.f);
}

void sameSlotOnStageOneKeepsTheFirstRate()
{
    argon::TextureBindCache cache;
    cache.bindMesh(modelA, twoStages(14, 60.f));

    cache.bind(1, argon::StageBind{argon::TextureSlot{modelA, 14}, 30.f});

    CHECK(cache.bound(1)->rate == 60.f);
}

void meshWithFewerStagesEmptiesTheRest()
{
    argon::TextureBindCache cache;
    cache.bindMesh(modelA, twoStages(14, 60.f));

    cache.bindMesh(modelA, oneStage(3, 0.f));

    CHECK(!cache.bound(1));
}

void emptiedStageRebinds()
{
    argon::TextureBindCache cache;
    cache.bindMesh(modelA, oneStage(14, 60.f));
    cache.empty(0);

    cache.bindMesh(modelA, oneStage(14, 30.f));

    CHECK(cache.bound(0)->rate == 30.f);
}

void meshBetweenWithFewerStagesBreaksTheRun()
{
    argon::TextureBindCache cache;
    cache.bindMesh(modelA, twoStages(14, 60.f));
    cache.bindMesh(modelA, oneStage(3, 0.f));

    cache.bindMesh(modelA, twoStages(14, 30.f));

    CHECK(cache.bound(1)->rate == 30.f);
}

void emptyAllEmptiesEveryStage()
{
    argon::TextureBindCache cache;
    cache.bindMesh(modelA, twoStages(14, 60.f));

    cache.emptyAll();

    CHECK(!cache.bound(0) && !cache.bound(1));
}

void newCacheHasNothingBound()
{
    const argon::TextureBindCache cache;

    CHECK(!cache.bound(0) && !cache.bound(3));
}
} // namespace

int main()
{
    firstBindKeepsItsRate();
    sameSlotKeepsTheFirstMeshsRate();
    otherTextureRebinds();
    sameTextureOfAnotherModelRebinds();
    sameSlotOnAnotherStageBindsThatStage();
    sameSlotOnStageOneKeepsTheFirstRate();
    meshWithFewerStagesEmptiesTheRest();
    emptiedStageRebinds();
    meshBetweenWithFewerStagesBreaksTheRun();
    emptyAllEmptiesEveryStage();
    newCacheHasNothingBound();
    std::printf("texture_bind_cache_tests: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
