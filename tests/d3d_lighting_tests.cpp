// Argon: tests for the game's mesh lighting (render\d3d_lighting.hpp: the surface state 0xba4f0, the setups 0xba370 /
// 0xba1d0, the craft lights 0x59bc0). Run with test.bat / test.sh.
#include "render/d3d_lighting.hpp"
#include "check.hpp"

#include <cmath>
#include <cstdio>

namespace
{
using argon::test::checks;
using argon::test::failures;

bool near(const glm::vec3 &a, const glm::vec3 &b)
{
    return glm::length(a - b) < 1e-4f;
}

argon::SurfaceSetup surface(uint32_t setup, uint32_t flags)
{
    argon::SurfaceSetup s;
    s.hasBlock = true;
    s.setup = setup;
    s.flags = flags;
    s.hasBoneRecord = true;
    return s;
}

glm::vec3 bytes(int r, int g, int b)
{
    return glm::vec3(float(r), float(g), float(b)) / 255.f;
}

void setupOneIsUnlitWithout0x8000()
{
    CHECK(!argon::surfaceLighting(surface(1, 0x21)).lit);
}

void setupOneWith0x8000IsLit()
{
    CHECK(argon::surfaceLighting(surface(1, 0x8021)).lit);
}

void setupTwoIsUnlitWith0x20()
{
    CHECK(!argon::surfaceLighting(surface(2, 0x23)).lit);
}

void setupTwoWithout0x20IsLit()
{
    const argon::SurfaceSetup titleLogoFace = surface(2, 0x3);

    CHECK(argon::surfaceLighting(titleLogoFace).lit);
}

void lensFlareIsLitByItsOwnColourAlone()
{
    const argon::SurfaceLighting l = argon::surfaceLighting(surface(1, 0x1bacb5));
    CHECK(l.lit && l.ambient == argon::LightSource::Own && l.light0 == argon::LightSource::Black);
}

void flag2000With100BlacksBothThenFlag4LightsLight0()
{
    const argon::SurfaceLighting l = argon::surfaceLighting(surface(2, 0x2105));
    CHECK(l.ambient == argon::LightSource::Black && l.light0 == argon::LightSource::Own);
}

void flag2000With200KeepsTheSun()
{
    CHECK(argon::surfaceLighting(surface(2, 0x2201)).light0 == argon::LightSource::Scene);
}

void ownColourNeedsTheBoneRecord()
{
    argon::SurfaceSetup s = surface(2, 0x5);
    s.hasBoneRecord = false;
    CHECK(argon::surfaceLighting(s).ambient == argon::LightSource::Scene);
}

void craftAmbientIsHalfwayToHalfTheGround()
{
    argon::SceneLights track{bytes(80, 80, 80), bytes(200, 200, 200)};
    const argon::SceneLights craft = argon::craftLights(0xffc86400u, track);
    CHECK(near(craft.ambient, bytes((100 + 80) / 2, (50 + 80) / 2, (0 + 80) / 2)));
}

void craftLightIsTheMeanCappedByTheSun()
{
    argon::SceneLights track{bytes(0, 0, 0), bytes(200, 100, 200)};
    const argon::SceneLights craft = argon::craftLights(0xffff0000u, track);
    CHECK(near(craft.light0, bytes(200, 50, 50)));
}

void craftLightBlueTakesTheGreenSum()
{
    argon::SceneLights track{bytes(0, 0, 0), bytes(255, 255, 255)};
    const argon::SceneLights craft = argon::craftLights(0xff00ff00u, track);
    CHECK(near(craft.light0, bytes(127, 255, 255)));
}
} // namespace

int main()
{
    setupOneIsUnlitWithout0x8000();
    setupOneWith0x8000IsLit();
    setupTwoIsUnlitWith0x20();
    setupTwoWithout0x20IsLit();
    lensFlareIsLitByItsOwnColourAlone();
    flag2000With100BlacksBothThenFlag4LightsLight0();
    flag2000With200KeepsTheSun();
    ownColourNeedsTheBoneRecord();
    craftAmbientIsHalfwayToHalfTheGround();
    craftLightIsTheMeanCappedByTheSun();
    craftLightBlueTakesTheGreenSum();
    std::printf("d3d_lighting_tests: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
