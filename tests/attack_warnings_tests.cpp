// Argon: tests for ATTACK WARNING's rules (logic/attack_warnings.hpp: the danger flag +0xb5 set by 0x66e60 and
// 0x683b0, shown by the power-up panel 0x4d720). No window or GPU: run with test.bat / test.sh (or ctest in build).
#include "logic/attack_warnings.hpp"
#include "check.hpp"

#include <cmath>
#include <cstdio>

namespace
{
using argon::test::checks;
using argon::test::failures;

using namespace argon;

constexpr float kHitRadius = 5.f;
const glm::vec3 kStart(0.f), kEnd(0.f, 0.f, 100.f);

bool near(float a, float b)
{
    return std::abs(a - b) < 1e-4f;
}

void testThePathOffsetMeasuresTheDistanceOffTheLine()
{
    CHECK(near(pathOffset(kStart, kEnd, glm::vec3(3.f, 0.f, 40.f)).distance, 3.f));
}

void testThePathOffsetMeasuresTheFootAlongThePath()
{
    CHECK(near(pathOffset(kStart, kEnd, glm::vec3(3.f, 0.f, 40.f)).along, 40.f));
}

void testAZeroLengthPathMeasuresFromItsStart()
{
    CHECK(near(pathOffset(kStart, kStart, glm::vec3(0.f, 4.f, 0.f)).distance, 4.f));
}

void testACraftInsideTheHitRadiusAlongThePathIsWarned()
{
    CHECK(pathWarns(pathOffset(kStart, kEnd, glm::vec3(4.f, 0.f, 50.f)), kHitRadius));
}

void testACraftOutsideTheHitRadiusIsNotWarned()
{
    CHECK(!pathWarns(pathOffset(kStart, kEnd, glm::vec3(6.f, 0.f, 50.f)), kHitRadius));
}

void testACraftBehindThePathsStartIsNotWarned()
{
    CHECK(!pathWarns(pathOffset(kStart, kEnd, glm::vec3(0.f, 0.f, -1.f)), kHitRadius));
}

void testACraftUpTo200mPastThePathsEndIsWarned()
{
    CHECK(pathWarns(pathOffset(kStart, kEnd, glm::vec3(0.f, 0.f, 299.f)), kHitRadius));
}

void testACraftMoreThan200mPastThePathsEndIsNotWarned()
{
    CHECK(!pathWarns(pathOffset(kStart, kEnd, glm::vec3(0.f, 0.f, 301.f)), kHitRadius));
}

void testAWarningShowsAfterItsStepEnds()
{
    AttackWarnings warnings;
    warnings.beginStep();
    warnings.warn(2);
    CHECK(!warnings.shown(2));
    warnings.endStep();
    CHECK(warnings.shown(2));
}

void testAStepWithoutDangerClearsTheWarning()
{
    AttackWarnings warnings;
    warnings.beginStep();
    warnings.warn(2);
    warnings.endStep();
    warnings.beginStep();
    warnings.endStep();
    CHECK(!warnings.shown(2));
}

void testResetClearsTheShownWarnings()
{
    AttackWarnings warnings;
    warnings.warn(1);
    warnings.endStep();
    warnings.reset();
    CHECK(!warnings.shown(1));
}

void testASlotOutOfRangeIsIgnored()
{
    AttackWarnings warnings;
    warnings.warn(-1);
    warnings.warn(16);
    warnings.endStep();
    CHECK(!warnings.shown(-1) && !warnings.shown(16));
}
}  // namespace

int main()
{
    testThePathOffsetMeasuresTheDistanceOffTheLine();
    testThePathOffsetMeasuresTheFootAlongThePath();
    testAZeroLengthPathMeasuresFromItsStart();
    testACraftInsideTheHitRadiusAlongThePathIsWarned();
    testACraftOutsideTheHitRadiusIsNotWarned();
    testACraftBehindThePathsStartIsNotWarned();
    testACraftUpTo200mPastThePathsEndIsWarned();
    testACraftMoreThan200mPastThePathsEndIsNotWarned();
    testAWarningShowsAfterItsStepEnds();
    testAStepWithoutDangerClearsTheWarning();
    testResetClearsTheShownWarnings();
    testASlotOutOfRangeIsIgnored();
    std::printf("attack_warnings_tests: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
