// Argon: tests for the scene constants and the views' areas (render\scene_constants.hpp, render\scene_views.hpp).
// Run with test.bat / test.sh.
#include "render/scene_constants.hpp"
#include "render/scene_views.hpp"
#include "check.hpp"

#include <cmath>
#include <cstdio>

namespace
{
using argon::test::checks;
using argon::test::failures;

bool nearlyEqual(float a, float b)
{
    return std::abs(a - b) < 1e-5f;
}

void zeroSunFallsBackToTheDefault()
{
    argon::TrackProps props;
    props.sunVector = {0.f, 0.f, 0.f};

    const glm::vec3 sun = sunDirection(props);

    CHECK(nearlyEqual(sun.y, -1.f / 1.5f));
}

void sunIsUnitLength()
{
    argon::TrackProps props;
    props.sunVector = {0.f, -4.f, 3.f};

    const glm::vec3 sun = sunDirection(props);

    CHECK(nearlyEqual(glm::length(sun), 1.f));
}

void shortFogRangeEndsAtTheDefault()
{
    argon::TrackProps props;
    props.fogRange = 1.f;

    const SceneUbo constants = raceSceneConstants(props, SceneToggles{}, SceneMoment{});

    CHECK(constants.fogParams.z == 3000.f);
}

void fogRangeEndsTheFog()
{
    argon::TrackProps props;
    props.fogRange = 1200.f;

    const SceneUbo constants = raceSceneConstants(props, SceneToggles{}, SceneMoment{});

    CHECK(constants.fogParams.z == 1200.f);
}

void noFogModeIsModeOne()
{
    const SceneUbo constants = raceSceneConstants(argon::TrackProps{}, SceneToggles{}, SceneMoment{});

    CHECK(constants.fogParams.x == 1.f);
}

void momentGivesTheEyeAndClock()
{
    const SceneMoment moment{glm::mat4(1.f), glm::vec3(1.f, 2.f, 3.f), 7.5f, std::nullopt};

    const SceneUbo constants = raceSceneConstants(argon::TrackProps{}, SceneToggles{}, moment);

    CHECK(constants.camTime == glm::vec4(1.f, 2.f, 3.f, 7.5f));
}

void withoutAStoryLightLightOneIsBlack()
{
    const SceneUbo constants = raceSceneConstants(argon::TrackProps{}, SceneToggles{}, SceneMoment{});

    CHECK(constants.light1Colour == glm::vec4(0.f));
}

void storyLightIsLightOne()
{
    SceneMoment moment;
    moment.light1 = argon::story::SceneLight{glm::vec3(0.f, -1.f, 0.f), glm::vec3(0.5f, 0.25f, 1.f)};

    const SceneUbo constants = raceSceneConstants(argon::TrackProps{}, SceneToggles{}, moment);

    CHECK(constants.light1Colour == glm::vec4(0.5f, 0.25f, 1.f, 1.f));
}

void frontEndAmbientIsTheGames()
{
    const SceneUbo constants = frontEndSceneConstants(FrontEndLights{}, SceneToggles{});

    CHECK(nearlyEqual(constants.ambient.r, 0x70 / 255.f));
}

void lockedCharacterIsUnlit()
{
    FrontEndLights lights;
    lights.lightsOff = true;

    const SceneUbo constants = frontEndSceneConstants(lights, SceneToggles{});

    CHECK(constants.sunColour == glm::vec4(0.f, 0.f, 0.f, 1.f));
}

void frontEndHasNoFog()
{
    const SceneUbo constants = frontEndSceneConstants(FrontEndLights{}, SceneToggles{});

    CHECK(constants.toggles.x == 0);
}

void wholeScreenCoversTheWindow()
{
    const argon::gfx::Rect area = screenArea({1280, 720}, argon::split::viewOf(1, 0));

    CHECK(area.x == 0 && area.y == 0 && area.width == 1280 && area.height == 720);
}

void lowerHalfStartsMidWindow()
{
    const argon::gfx::Rect area = screenArea({1280, 721}, argon::split::viewOf(2, 1));

    CHECK(area.y + int32_t(area.height) == 721);
}

void quartersMeetWithoutAGap()
{
    const argon::gfx::Rect left = screenArea({1279, 720}, argon::split::viewOf(4, 0));
    const argon::gfx::Rect right = screenArea({1279, 720}, argon::split::viewOf(4, 1));

    CHECK(left.x + int32_t(left.width) == right.x);
}

void halfScreenViewIsTwiceAsWide()
{
    const float aspect = viewAspect({1280, 720}, argon::split::viewOf(2, 0));

    CHECK(nearlyEqual(aspect, 2.f * 1280.f / 720.f));
}

void zeroHeightWindowHasAnAspect()
{
    CHECK(windowAspect({640, 0}) == 640.f);
}

}  // namespace

int main()
{
    zeroSunFallsBackToTheDefault();
    sunIsUnitLength();
    shortFogRangeEndsAtTheDefault();
    fogRangeEndsTheFog();
    noFogModeIsModeOne();
    momentGivesTheEyeAndClock();
    withoutAStoryLightLightOneIsBlack();
    storyLightIsLightOne();
    frontEndAmbientIsTheGames();
    lockedCharacterIsUnlit();
    frontEndHasNoFog();
    wholeScreenCoversTheWindow();
    lowerHalfStartsMidWindow();
    quartersMeetWithoutAGap();
    halfScreenViewIsTwiceAsWide();
    zeroHeightWindowHasAnAspect();
    std::printf("scene_frame_tests: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
