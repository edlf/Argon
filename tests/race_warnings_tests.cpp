// Argon: tests for the HUD's warnings (logic/race_warnings.hpp): RESET's 0.5 s after a respawn, WRONG WAY
// after over a second against the route, "Shield Low" every second under 33 %. Run with test.bat / test.sh.
#include "logic/race_warnings.hpp"
#include "check.hpp"

#include <cstdio>

namespace
{
using argon::test::checks;
using argon::test::failures;

constexpr float kStep = 1.f / 60.f;

argon::RaceWarnings::Step heading(float forwardZ)
{
    argon::RaceWarnings::Step s;
    s.forward = glm::vec3(0, 0, forwardZ);
    s.route = glm::vec3(0, 0, 10.f);
    return s;
}

void stepFor(argon::RaceWarnings &w, const argon::RaceWarnings::Step &s, int steps)
{
    for (int i = 0; i < steps; ++i)
    {
        w.step(s, kStep);
    }
}

void resetShowsForHalfASecondAfterARespawn()
{
    argon::RaceWarnings w;
    argon::RaceWarnings::Step s = heading(1.f);
    s.respawned = true;
    w.step(s, kStep);
    s.respawned = false;
    stepFor(w, s, 28);
    CHECK(w.resetTime() > 0.f);
    stepFor(w, s, 1);
    CHECK(w.resetTime() == 0.f);
}

void wrongWayNeedsOverASecondAgainstTheRoute()
{
    argon::RaceWarnings w;
    stepFor(w, heading(-1.f), 60);
    CHECK(!w.wrongWay());
    stepFor(w, heading(-1.f), 2);
    CHECK(w.wrongWay());
}

void wrongWayClearsWhenTheCraftTurnsBack()
{
    argon::RaceWarnings w;
    stepFor(w, heading(-1.f), 90);
    stepFor(w, heading(-0.3f), 1);
    CHECK(!w.wrongWay());
}

void wrongWayWaitsWhileResetShows()
{
    argon::RaceWarnings w;
    argon::RaceWarnings::Step s = heading(-1.f);
    s.respawned = true;
    w.step(s, kStep);
    s.respawned = false;
    stepFor(w, s, 80);
    CHECK(!w.wrongWay());
}

void shieldLowPostsOnceASecondWhileLow()
{
    argon::ShieldLowWarning w;
    int posted = 0;
    for (int i = 0; i < 60 * 3 + 30; ++i)
    {
        w.step(0.2f, kStep);
        if (w.popupDue())
        {
            ++posted;
        }
    }
    CHECK(posted == 3);
}

void shieldLowStaysQuietAboveAThird()
{
    argon::ShieldLowWarning w;
    int posted = 0;
    for (int i = 0; i < 60 * 3; ++i)
    {
        w.step(0.5f, kStep);
        if (w.popupDue())
        {
            ++posted;
        }
    }
    CHECK(posted == 0);
}
}  // namespace

int main()
{
    resetShowsForHalfASecondAfterARespawn();
    wrongWayNeedsOverASecondAgainstTheRoute();
    wrongWayClearsWhenTheCraftTurnsBack();
    wrongWayWaitsWhileResetShows();
    shieldLowPostsOnceASecondWhileLow();
    shieldLowStaysQuietAboveAThird();
    std::printf("race warnings tests: %d checks, %d failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
