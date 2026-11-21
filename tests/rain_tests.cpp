// Argon: tests for the race's rain (logic/rain.hpp). Run with test.bat / test.sh.
#include "logic/rain.hpp"
#include "check.hpp"

#include <cmath>
#include <cstdio>

using namespace argon::rain;

namespace
{
using argon::test::checks;
using argon::test::failures;

bool near(float a, float b, float eps = 1e-4f)
{
    return std::abs(a - b) <= eps;
}

int liveDrops(const Rain &rain)
{
    int live = 0;
    for (const LensDrop &drop : rain.lensDrops().drops())
    {
        live += drop.size > 0.f;
    }
    return live;
}

Rain rainAfterFrames(const Weather &weather, int frames, const RainCamera &camera = {})
{
    Rain rain;
    for (int i = 0; i < frames; ++i)
    {
        rain.setWeather(weather);
        rain.step(camera);
    }
    return rain;
}

void kalauAndAltBrackenRain()
{
    CHECK(raceRains(1, false) && raceRains(0xd, false));
}

void otherTracksStayDry()
{
    CHECK(!raceRains(0, false) && !raceRains(5, false) && !raceRains(9, false));
}

void alwaysRainMakesEveryTrackRain()
{
    CHECK(raceRains(0, true) && raceRains(0x10, true));
}

void gameRandMatchesTheCRuntimeSequence()
{
    GameRand rand(1);
    const int first = rand.next(), second = rand.next(), third = rand.next();
    CHECK(first == 41 && second == 18467 && third == 6334);
}

void twelveDropsLandEachRainingFrame()
{
    const Rain rain = rainAfterFrames({true, false, false}, 1);
    CHECK(liveDrops(rain) == 12);
}

void noDropsLandUnderARoof()
{
    const Rain rain = rainAfterFrames({true, true, false}, 10);
    CHECK(liveDrops(rain) == 0);
}

void divingClearsTheLens()
{
    Rain rain = rainAfterFrames({true, false, false}, 10);
    rain.setWeather({true, false, true});
    CHECK(liveDrops(rain) == 0);
}

void noRainFallsUnderWater()
{
    const Rain rain = rainAfterFrames({true, false, true}, 10);
    CHECK(liveDrops(rain) == 0 && rain.streakAlpha() == 0 && !rain.cloudShown());
}

void surfacingWetsTheWholeLens()
{
    Rain rain = rainAfterFrames({false, false, true}, 1);
    rain.setWeather({false, false, false});
    CHECK(liveDrops(rain) == Rain::kDrops);
}

Rain surfacedOnADryTrack(int framesAfter)
{
    Rain rain = rainAfterFrames({false, false, true}, 1);
    for (int i = 0; i < framesAfter; ++i)
    {
        rain.setWeather({false, false, false});
        rain.step({});
    }
    return rain;
}

void aDryTrackKeepsTheWetLensForSeconds()
{
    const Rain rain = surfacedOnADryTrack(100);
    CHECK(liveDrops(rain) == Rain::kDrops);
}

void theWetLensDriesWithinTenSeconds()
{
    const Rain rain = surfacedOnADryTrack(600);   // the biggest drop, 0.11, dries at 0.0002 a frame
    CHECK(liveDrops(rain) == 0);
}

void sprayNearTheEyeBringsBackTwentyOneDrops()
{
    Rain rain;
    rain.splash();
    CHECK(liveDrops(rain) == 21);
}

void sprayWithinTwoMetresOnEveryAxisReachesTheLens()
{
    const glm::vec3 eye(10.f, 5.f, -3.f);
    CHECK(sprayReachesLens(eye, eye + glm::vec3(1.9f, -1.9f, 1.9f)));
}

void sprayTwoMetresAwayOnOneAxisMissesTheLens()
{
    const glm::vec3 eye(10.f, 5.f, -3.f);
    CHECK(!sprayReachesLens(eye, eye + glm::vec3(0.f, 0.f, -2.f)));
}

void newDropsLandOnTheViewWithTheirSize()
{
    const Rain rain = rainAfterFrames({true, false, false}, 1);
    const ViewRect view;
    bool allFit = true;
    for (const LensDrop &drop : rain.lensDrops().drops())
    {
        if (drop.size > 0.f)
        {
            allFit = allFit && view.contains(drop.pos) && drop.size >= 0.03f - 0.0002f && drop.size <= 0.11f;
        }
    }
    CHECK(allFit);
}

void streaksAreHalfTheStrengthWhileRaining()
{
    const Rain rain = rainAfterFrames({true, false, false}, 1);
    CHECK(rain.streakAlpha() == 35);
}

void streaksVanishWhenDry()
{
    const Rain rain = rainAfterFrames({false, false, false}, 1);
    CHECK(rain.streakAlpha() == 0);
}

void streaksFallAroundAPointSixteenMetresAhead()
{
    RainCamera camera;
    camera.eye = {100.f, 10.f, -50.f};
    camera.back = {0.f, 0.f, 1.f};
    std::vector<Streak> streaks(500);
    GameRand rand;
    scatterStreaks(streaks, camera, rand);
    glm::vec3 sum(0.f);
    for (const Streak &s : streaks)
    {
        sum += (s.head + s.tail) * 0.5f;
    }
    const glm::vec3 mean = sum / float(streaks.size());
    CHECK(near(mean.x, 100.f, 2.5f) && near(mean.y, 10.f, 2.5f) && near(mean.z, -66.f, 2.5f));
}

void aStandingStreakHangsTwoMetresBelowItsTail()
{
    RainCamera camera;
    std::vector<Streak> streaks(1);
    GameRand rand;
    scatterStreaks(streaks, camera, rand);
    CHECK(near(streaks[0].tail.y - streaks[0].head.y, 4.f));
}

void movingTiltsTheStreaksBack()
{
    RainCamera camera;
    camera.velocity = {0.f, 0.f, -100.f};
    std::vector<Streak> streaks(1);
    GameRand rand;
    scatterStreaks(streaks, camera, rand);
    CHECK(streaks[0].head.z - streaks[0].tail.z > 2.f);
}

void aStillDropShowsTheViewTurnedOverFourTimesAsWide()
{
    LensDrops drops(1, ViewRect{});
    LensDrop drop;
    drop.pos = {320.f, 240.f};
    drop.size = 0.05f;
    const DropFan fan = drops.fan(drop);
    const glm::vec2 rim = fan[1].pos - fan[0].pos, texel = fan[1].texel - fan[0].texel;
    CHECK(near(rim.y, 3.2f) && near(texel.y, -12.8f));
}

void aRunningDropTrailsItsTailBehind()
{
    LensDrops drops(1, ViewRect{});
    LensDrop drop;
    drop.pos = {320.f, 240.f};
    drop.size = 0.1f;
    drop.angleDeg = 0.f;   // running down the screen
    drop.stretch = 2.f;
    const DropFan fan = drops.fan(drop);
    CHECK(near(fan[5].pos.y - 240.f, -0.1f * 64.f * 3.f));
}

void fastCameraPushesBigDropsOutward()
{
    RainCamera camera;
    camera.velocity = {0.f, 0.f, -300.f};   // flying forward along -z, the back axis +z
    Rain rain;
    float before = 0.f, after = 0.f;
    for (int i = 0; i < 200; ++i)
    {
        rain.setWeather({true, false, false});
        rain.step(camera);
        if (i == 0)
        {
            for (const LensDrop &drop : rain.lensDrops().drops())
            {
                before += drop.size > 0.08f ? std::abs(drop.vel.x) : 0.f;
            }
        }
    }
    for (const LensDrop &drop : rain.lensDrops().drops())
    {
        after += drop.size > 0.08f ? std::abs(drop.vel.x) : 0.f;
    }
    CHECK(before == 0.f && after > 0.f);
}

void cloudScrollsAndWrapsAtTheTextureRepeat()
{
    RainCloud cloud;
    GameRand rand;
    cloud.step(rand);
    const glm::vec4 t = cloud.texels();
    CHECK(near(t.x, (1024.f - 0.06f) * 0.125f, 1e-2f) && near(t.w - t.y, 0.25f));
}

void cloudHazeIsFaintGreyStandingStill()
{
    RainCloud cloud;
    const uint32_t c = cloud.colour(0.f, 90.f);
    CHECK((c & 0xffffffu) == 0xa0a0a0u && (c >> 24) == 18u);
}

void cloudHazeThickensWithSpeed()
{
    RainCloud cloud;
    const uint32_t still = cloud.colour(0.f, 90.f) >> 24, fast = cloud.colour(300.f * 2e-5f, 90.f) >> 24;
    CHECK(fast == 40u && fast > still);
}

void cloudHidesUnderARoof()
{
    const Rain rain = rainAfterFrames({true, true, false}, 1);
    CHECK(!rain.cloudShown());
}
}  // namespace

int main()
{
    kalauAndAltBrackenRain();
    otherTracksStayDry();
    alwaysRainMakesEveryTrackRain();
    gameRandMatchesTheCRuntimeSequence();
    twelveDropsLandEachRainingFrame();
    noDropsLandUnderARoof();
    divingClearsTheLens();
    noRainFallsUnderWater();
    surfacingWetsTheWholeLens();
    aDryTrackKeepsTheWetLensForSeconds();
    theWetLensDriesWithinTenSeconds();
    sprayNearTheEyeBringsBackTwentyOneDrops();
    sprayWithinTwoMetresOnEveryAxisReachesTheLens();
    sprayTwoMetresAwayOnOneAxisMissesTheLens();
    newDropsLandOnTheViewWithTheirSize();
    streaksAreHalfTheStrengthWhileRaining();
    streaksVanishWhenDry();
    streaksFallAroundAPointSixteenMetresAhead();
    aStandingStreakHangsTwoMetresBelowItsTail();
    movingTiltsTheStreaksBack();
    aStillDropShowsTheViewTurnedOverFourTimesAsWide();
    aRunningDropTrailsItsTailBehind();
    fastCameraPushesBigDropsOutward();
    cloudScrollsAndWrapsAtTheTextureRepeat();
    cloudHazeIsFaintGreyStandingStill();
    cloudHazeThickensWithSpeed();
    cloudHidesUnderARoof();
    std::printf("rain_tests: %d checks, %d failed\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
