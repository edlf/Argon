// Argon: tests for the frames' time in 60 Hz ticks (logic/tick_accumulator.hpp). Run with test.bat / test.sh.
#include "logic/tick_accumulator.hpp"
#include "check.hpp"

#include <cmath>
#include <cstdio>

namespace
{
using argon::test::checks;
using argon::test::failures;

using argon::kGameTick;
using argon::TickAccumulator;

int ticksTaken(TickAccumulator &ticks)
{
    int taken = 0;
    for (; ticks.tickDue(); ++taken)
    {
        ticks.takeTick();
    }
    return taken;
}

void lessThanATickIsNotDue()
{
    TickAccumulator ticks;

    ticks.add(kGameTick * 0.5f);

    CHECK(!ticks.tickDue());
}

void aFrameOfThreeTicksGivesThree()
{
    TickAccumulator ticks;
    ticks.add(kGameTick * 3.5f);

    const int taken = ticksTaken(ticks);

    CHECK(taken == 3);
}

void theRestIsTheShareOfTheNextTick()
{
    TickAccumulator ticks;
    ticks.add(kGameTick * 2.25f);

    ticksTaken(ticks);

    CHECK(std::fabs(ticks.tickShare() - 0.25f) < 1e-3f);
}

void theShareStopsAtAWholeTick()
{
    TickAccumulator ticks;

    ticks.add(kGameTick * 5.f);

    CHECK(ticks.tickShare() == 1.f);
}

void aRestartDropsWhatWasPending()
{
    TickAccumulator ticks;
    ticks.add(kGameTick * 4.f);

    ticks.restart();

    CHECK(!ticks.tickDue() && ticks.tickShare() == 0.f);
}

}  // namespace

int main()
{
    lessThanATickIsNotDue();
    aFrameOfThreeTicksGivesThree();
    theRestIsTheShareOfTheNextTick();
    theShareStopsAtAWholeTick();
    aRestartDropsWhatWasPending();
    std::printf("tick_accumulator_tests: %d checks, %d failed\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
