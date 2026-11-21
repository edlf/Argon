// Argon: tests for the shaders cross-compiled for Direct3D 12 (gfx/d3d12/d3d12_shaders.hpp): every one a DXIL
// container. No window or GPU: run with test.bat (or ctest in build).
#include "gfx/d3d12/d3d12_shaders.hpp"
#include "check.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iterator>

namespace
{
using argon::test::checks;
using argon::test::failures;

// a DXBC container ("DXBC", the hash, version 1, its size) whose parts include the DXIL program
template <size_t N> bool isDxilContainer(const unsigned char (&blob)[N])
{
    const char kDxil[4] = {'D', 'X', 'I', 'L'};
    if (N < 32 || std::memcmp(blob, "DXBC", 4) != 0)
    {
        return false;
    }
    uint32_t size = 0;
    std::memcpy(&size, blob + 24, 4);
    return size == N && std::search(std::begin(blob), std::end(blob), std::begin(kDxil), std::end(kDxil)) != std::end(blob);
}

void testTheMeshProgramIsDxil()
{
    CHECK(isDxilContainer(kMeshVertDxil) && isDxilContainer(kMeshFragDxil));
}

void testTheEffectProgramsAreDxil()
{
    CHECK(isDxilContainer(kWaterVertDxil) && isDxilContainer(kWaterFragDxil) && isDxilContainer(kTrailVertDxil) && isDxilContainer(kTrailFragDxil) &&
          isDxilContainer(kSprayFragDxil) && isDxilContainer(kQuadSpriteVertDxil) && isDxilContainer(kQuadSpriteFragDxil));
}

void testTheShadowProgramsAreDxil()
{
    CHECK(isDxilContainer(kShadowVertDxil) && isDxilContainer(kShadowFragDxil) && isDxilContainer(kShadowCastFragDxil) &&
          isDxilContainer(kShadowSolidFragDxil));
}

void testTheLineOcclusionAndHudProgramsAreDxil()
{
    CHECK(isDxilContainer(kLineVertDxil) && isDxilContainer(kLineFragDxil) && isDxilContainer(kOcclVertDxil) && isDxilContainer(kOcclFragDxil) &&
          isDxilContainer(kHudAddVertDxil) && isDxilContainer(kHudAddFragDxil) && isDxilContainer(kHudPanelFragDxil));
}

void testTheAntiAliasingProgramsAreDxil()
{
    CHECK(isDxilContainer(kFullscreenVertDxil) && isDxilContainer(kFxaaFragDxil) && isDxilContainer(kSmaaEdgesFragDxil) &&
          isDxilContainer(kSmaaWeightsFragDxil) && isDxilContainer(kSmaaBlendFragDxil));
}
}  // namespace

int main()
{
    testTheMeshProgramIsDxil();
    testTheEffectProgramsAreDxil();
    testTheShadowProgramsAreDxil();
    testTheLineOcclusionAndHudProgramsAreDxil();
    testTheAntiAliasingProgramsAreDxil();
    std::printf("%d checks, %d failed\n", checks, failures);
    return failures ? 1 : 0;
}
