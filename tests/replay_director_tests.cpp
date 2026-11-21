// Argon: tests for the replay cameras' director (logic/replay_director.hpp, the game's CReplay 0xd6340): the
// cameras' scores, hold times and switches. No window, GPU or game files: run with test.bat / test.sh (or ctest).
#include <cmath>
#include <cstdio>

#include "logic/replay_director.hpp"
#include "check.hpp"

namespace
{
using argon::test::checks;
using argon::test::failures;

using namespace argon::replay;

constexpr float kFrame = 1.f / 60.f;

argon::TrackCamera staticAt(float x, float panFrom, float panTo, float hold)
{
    argon::TrackCamera c;
    c.type = "static";
    c.pos = {x, 10.f, 0.f};
    c.target = {x, 10.f, 1.f};
    c.range = 100.f;
    c.hold = hold;
    c.panFrom = panFrom;
    c.panTo = panTo;
    return c;
}

FollowedCraft craftAt(float x, float lapPercent)
{
    FollowedCraft craft;
    craft.pos = {x, 0.f, 20.f};
    craft.lapPercent = lapPercent;
    return craft;
}

// two statics 200 m apart, both covering the whole lap
ReplayDirector twoStatics(float hold)
{
    ReplayDirector director;
    director.reset({staticAt(0.f, 0.f, 100.f, hold), staticAt(200.f, 0.f, 100.f, hold)}, 2);
    return director;
}

void theFirstStaticStartsTheReplay()
{
    ReplayDirector director = twoStatics(2.f);
    director.step({craftAt(0.f, 10.f)}, kFrame);
    CHECK(director.current() == 0);
}

void theNearestCameraTakesOverOnceTheHoldIsOver()
{
    ReplayDirector director = twoStatics(0.f);
    director.step({craftAt(0.f, 10.f)}, kFrame);
    director.step({craftAt(200.f, 20.f)}, kFrame);
    CHECK(director.current() == 1);
}

void aHeldCameraKeepsTheView()
{
    ReplayDirector director = twoStatics(0.f);
    director.step({craftAt(200.f, 10.f)}, kFrame);   // switches to 1, held for at least 1 s
    director.step({craftAt(0.f, 20.f)}, kFrame);
    CHECK(director.current() == 1);
}

void theHoldLastsAtLeastASecond()
{
    ReplayDirector director = twoStatics(0.f);
    director.step({craftAt(200.f, 10.f)}, kFrame);
    for (int frame = 0; frame < 61; ++frame)
    {
        director.step({craftAt(0.f, 20.f)}, kFrame);
    }
    CHECK(director.current() == 0);
}

void aCameraOutsideItsLapWindowIsNotPicked()
{
    ReplayDirector director;
    director.reset({staticAt(0.f, 0.f, 100.f, 0.f), staticAt(200.f, 50.f, 60.f, 0.f)}, 2);
    director.step({craftAt(200.f, 10.f)}, kFrame);
    CHECK(director.current() == 0);
}

void aWindowPastTheLineWraps()
{
    ReplayDirector director;
    director.reset({staticAt(0.f, 0.f, 100.f, 0.f), staticAt(200.f, 90.f, 5.f, 0.f)}, 2);
    director.step({craftAt(200.f, 2.f)}, kFrame);
    CHECK(director.current() == 1);
}

void aStaticCameraWithoutTrackingLooksAtItsTarget()
{
    ReplayDirector director = twoStatics(2.f);
    const CameraShot shot = director.step({craftAt(0.f, 10.f)}, kFrame);
    CHECK(shot.look == glm::vec3(0.f, 10.f, 1.f) && shot.eye == glm::vec3(0.f, 10.f, 0.f));
}

void aTrackingCameraTurnsTowardsTheCraft()
{
    argon::TrackCamera tracking = staticAt(0.f, 0.f, 100.f, 2.f);
    tracking.dynamicTracking = true;
    ReplayDirector director;
    director.reset({tracking}, 2);
    director.step({craftAt(0.f, 10.f)}, kFrame);
    const CameraShot shot = director.step({craftAt(10.f, 10.f)}, kFrame);
    CHECK(shot.look.x > 0.f && shot.look.x < 10.f && shot.look.y == 0.f);
}

void aDynamicFovNarrowsWithDistance()
{
    argon::TrackCamera zoom = staticAt(0.f, 0.f, 100.f, 2.f);
    zoom.dynamicFov = true;
    zoom.range = 1000.f;
    ReplayDirector director;
    director.reset({zoom}, 2);
    CameraShot shot;
    for (int frame = 0; frame < 600; ++frame)
    {
        shot = director.step({craftAt(0.f, 10.f)}, kFrame);   // 22.4 m away: 90 - 0.7 x 22.4
    }
    CHECK(std::abs(shot.fov - (90.f - 0.7f * std::sqrt(500.f))) < 0.1f);
}

void aShipCameraTakesOverWhenNothingIsInRange()
{
    argon::TrackCamera ship;
    ship.type = "ship";
    ship.pos = {-4.f, 0.f, 1.f};
    ReplayDirector director;
    director.reset({staticAt(0.f, 0.f, 100.f, 0.f), ship}, 2);
    director.step({craftAt(500.f, 10.f)}, kFrame);
    CHECK(director.current() == 1);
}

void aShipCameraSitsBehindTheCraft()
{
    argon::TrackCamera ship;
    ship.type = "ship";
    ship.pos = {-4.f, 0.f, 1.f};
    ReplayDirector director;
    director.reset({staticAt(0.f, 0.f, 100.f, 0.f), ship}, 2);
    const CameraShot shot = director.step({craftAt(500.f, 10.f)}, kFrame);
    CHECK(shot.eye == glm::vec3(500.f, 1.f, 16.f));
}

void aKerbCameraWantsAShakeFromRacersWithinTenMetres()
{
    const std::vector<PassingCraft> racers{{{5.f, 0.f, 0.f}, 100.f}, {{12.f, 0.f, 0.f}, 100.f}};
    CHECK(std::abs(KerbShake::wanted(glm::vec3(0.f), racers) - 1.25f) < 1e-5f);   // (10 - 5) x 0.05 x 100 x 0.05
}

void aKerbShakeStaysStillWithNobodyClose()
{
    KerbShake shake;
    shake.step(0.f, kFrame);
    CHECK(!shake.shaking());
}

void aKerbShakeIsSmallAndBounded()
{
    KerbShake shake;
    for (int frame = 0; frame < 120; ++frame)
    {
        shake.step(10.f, kFrame);
    }
    const float most = (0.6f + 5.f * kFrame) * 0.02f * std::sqrt(3.f);
    CHECK(shake.shaking() && glm::length(shake.offset()) <= most);
}

void aKerbShakeDiesDownOnceTheRacersHavePassed()
{
    KerbShake shake;
    shake.step(10.f, kFrame);
    for (int frame = 0; frame < 300; ++frame)
    {
        shake.step(0.f, kFrame);
    }
    CHECK(glm::length(shake.offset()) < 1e-3f);
}

void aShakingKerbCameraLooksAtItsTargetNotTheCraft()
{
    argon::TrackCamera kerb = staticAt(0.f, 0.f, 100.f, 2.f);
    kerb.dynamicTracking = true;
    kerb.kerb = true;
    ReplayDirector director;
    director.reset({kerb}, 2);
    const FollowedCraft craft = craftAt(3.f, 10.f);
    const CameraShot shot = director.step({craft, {{glm::vec3(3.f, 10.f, 0.f), 100.f}}}, kFrame);
    CHECK(glm::length(shot.look - glm::vec3(0.f, 10.f, 1.f)) < 0.05f);
}

}  // namespace

int main()
{
    theFirstStaticStartsTheReplay();
    theNearestCameraTakesOverOnceTheHoldIsOver();
    aHeldCameraKeepsTheView();
    theHoldLastsAtLeastASecond();
    aCameraOutsideItsLapWindowIsNotPicked();
    aWindowPastTheLineWraps();
    aStaticCameraWithoutTrackingLooksAtItsTarget();
    aTrackingCameraTurnsTowardsTheCraft();
    aDynamicFovNarrowsWithDistance();
    aShipCameraTakesOverWhenNothingIsInRange();
    aShipCameraSitsBehindTheCraft();
    aKerbCameraWantsAShakeFromRacersWithinTenMetres();
    aKerbShakeStaysStillWithNobodyClose();
    aKerbShakeIsSmallAndBounded();
    aKerbShakeDiesDownOnceTheRacersHavePassed();
    aShakingKerbCameraLooksAtItsTargetNotTheCraft();
    std::printf("%d checks, %d failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
