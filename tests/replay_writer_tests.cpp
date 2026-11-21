// Argon: tests for recording races in the game's replay format (logic/replay_writer.hpp, read back by
// logic/replay.hpp), the replays' lap count (logic/lap_tally.hpp) and the attract demo's idle clock
// (logic/attract_demo.hpp). No window, GPU or game files: run with test.bat / test.sh (or ctest in build).
#include <cmath>
#include <cstdio>
#include <cstring>

#include <glm/gtc/matrix_transform.hpp>

#include "logic/attract_demo.hpp"
#include "check.hpp"
#include "logic/lap_tally.hpp"
#include "logic/race_recording.hpp"
#include "logic/replay.hpp"
#include "logic/replay_weapons.hpp"
#include "logic/replay_writer.hpp"

namespace
{
using argon::test::checks;
using argon::test::failures;

using namespace argon::replay;

constexpr float kStep = 1.f / 60.f;

bool near(float a, float b, float tolerance = 1e-3f)
{
    return std::abs(a - b) < tolerance;
}

CraftEvent craftAt(const glm::vec3 &pos, const glm::vec3 &vel)
{
    CraftEvent e;
    e.pos = pos;
    e.vel = vel;
    return e;
}

Info twoCraftInfo()
{
    Info lineup;
    lineup.track = 5;        // bracken
    lineup.pilots[0] = 5;    // marcus
    lineup.pilots[1] = 13;   // jaya
    return lineup;
}

// two crafts flying straight on, craft 1 turning a little each frame; frames at 60 Hz from 1 / 60 s
RecordingFiles twoCraftRace(int frames)
{
    RecordingWriter writer;
    writer.start(twoCraftInfo(), 0.f);
    for (int f = 1; f <= frames; ++f)
    {
        const float t = kStep * float(f);
        CraftEvent straight = craftAt({0.f, 0.f, 50.f * t}, {0.f, 0.f, 50.f});
        CraftEvent turning = craftAt({5.f, 0.f, 40.f * t}, {0.f, 0.f, 40.f});
        turning.rotation = glm::mat3(glm::rotate(glm::mat4(1.f), 0.1f * float(f), glm::vec3(0.f, 1.f, 0.f)));
        turning.controls = ControlThrottle | ControlLeft;
        writer.recordFrame(t, {straight, turning});
    }
    writer.stop();
    return writer.files();
}

Recording readBack(const RecordingFiles &files)
{
    return parseRecording(files.lineupText, files.index, files.events);
}

// ---- the recording rule (0x3eee0)

void aCraftOnItsReckoningNeedsNoEntry()
{
    CraftEvent last = craftAt({0.f, 0.f, 0.f}, {10.f, 0.f, 0.f});
    CraftEvent now = craftAt({1.f, 0.f, 0.f}, {10.f, 0.f, 0.f});
    now.time = 0.1f;
    CHECK(!driftedFrom(last, now));
}

void aCraftOffItsReckoningNeedsAnEntry()
{
    CraftEvent last = craftAt({0.f, 0.f, 0.f}, {10.f, 0.f, 0.f});
    CraftEvent now = craftAt({1.2f, 0.f, 0.f}, {10.f, 0.f, 0.f});
    now.time = 0.1f;
    CHECK(driftedFrom(last, now));
}

void aVelocityChangeNeedsAnEntry()
{
    CraftEvent last = craftAt({0.f, 0.f, 0.f}, {10.f, 0.f, 0.f});
    CraftEvent now = craftAt({0.f, 0.f, 0.f}, {11.5f, 0.f, 0.f});
    CHECK(driftedFrom(last, now));
}

void aTurnNeedsAnEntry()
{
    CraftEvent last;
    CraftEvent now;
    now.rotation = glm::mat3(glm::rotate(glm::mat4(1.f), 0.06f, glm::vec3(0.f, 1.f, 0.f)));
    CHECK(driftedFrom(last, now));
}

void controlBitsFollowTheSlots()
{
    std::array<float, 13> slots{};
    slots[0] = 80.f;   // throttle
    slots[6] = 100.f;  // steer right
    slots[9] = 30.f;   // brake
    CHECK(controlBitsOf(slots, true) == (ControlThrottle | ControlFire | ControlRight | ControlBrake));
}

void framesCloserThanTheIntervalWriteNothing()
{
    RecordingWriter writer;
    writer.start(twoCraftInfo(), 0.f);
    writer.recordFrame(0.010f, {craftAt({0.f, 0.f, 0.f}, {0.f, 0.f, 0.f})});
    writer.recordFrame(0.020f, {craftAt({9.f, 0.f, 0.f}, {0.f, 0.f, 0.f})});
    writer.stop();
    const Recording r = readBack(writer.files());
    CHECK(r.crafts[0].size() == 1);
}

// ---- the files

void infoReadsBackTheTrackAndPilots()
{
    const Info lineup = parseInfo(formatInfo(twoCraftInfo()));
    CHECK(lineup.track == 5);
    CHECK(lineup.pilots[0] == 5);
    CHECK(lineup.pilots[1] == 13);
    CHECK(lineup.craftCount() == 2);
}

void eachFrameIsAnIndexEntry()
{
    const Recording r = readBack(twoCraftRace(30));
    CHECK(r.index.size() == 30);
}

// eventbuffer_advance: the entries of the frame at t are in the chunk timed by the frame before
void aChunkCarriesTheTimeOfTheFrameBefore()
{
    const Recording r = readBack(twoCraftRace(30));
    CHECK(near(r.crafts[0].front().time, 0.f, 1e-6f));
    CHECK(near(r.crafts[0].front().pos.z, 50.f * kStep, 1e-5f));
}

void positionsReadBackExactly()
{
    const Recording r = readBack(twoCraftRace(30));
    const CraftEvent &e = r.crafts[1].back();
    CHECK(e.pos == glm::vec3(5.f, 0.f, 40.f * kStep * 30.f));
    CHECK(e.vel == glm::vec3(0.f, 0.f, 40.f));
    CHECK(e.controls == (ControlThrottle | ControlLeft));
}

void rotationsReadBackToTheShortsPrecision()
{
    const Recording r = readBack(twoCraftRace(30));
    const glm::mat3 expected = glm::mat3(glm::rotate(glm::mat4(1.f), 3.f, glm::vec3(0.f, 1.f, 0.f)));
    const glm::mat3 &read = r.crafts[1].back().rotation;
    float worst = 0.f;
    for (int c = 0; c < 3; ++c)
    {
        for (int row = 0; row < 3; ++row)
        {
            worst = std::max(worst, std::abs(read[c][row] - expected[c][row]));
        }
    }
    CHECK(worst < 2.f / 4096.f);
}

void aStraightCraftIsWrittenOnlyWhenItDrifts()
{
    const Recording r = readBack(twoCraftRace(30));
    CHECK(r.crafts[0].size() == 1);    // on its dead reckoning all the way
    CHECK(r.crafts[1].size() == 30);   // turning 0.1 rad a frame
}

void entriesNameTheirCrafts()
{
    const RecordingFiles files = twoCraftRace(1);
    uint32_t target = 0;
    std::memcpy(&target, files.events.data() + 0x10 + 0xc, 4);
    CHECK(target == nameHash("craft0"));
}

void theIndexEndsWithTheMarker()
{
    const RecordingFiles files = twoCraftRace(3);
    float last = 0.f;
    std::memcpy(&last, files.index.data() + files.index.size() - 8, 4);
    CHECK(last == -1.f);
}

void aRecordedRacePlaysBackTheSamePoses()
{
    const Recording r = readBack(twoCraftRace(60));
    Pose pose;
    CHECK(poseAt(r.crafts[1], 0.5f, pose));
    CHECK(near(pose.pos.z, 40.f * (0.5f + kStep), 1e-3f));   // a frame ahead: the chunk's time is the frame before's
}

// ---- the replay's lap count

void lapsAddUpToTheRaceTime()
{
    LapTally tally;
    tally.update(0, 1, 30.f, 3);
    tally.update(0, 2, 31.f, 3);
    tally.update(0, 3, 29.5f, 3);
    CHECK(near(tally.raceTime(0), 90.5f));
}

void reachingTheLimitFinishesInOrder()
{
    LapTally tally;
    tally.update(2, 3, 30.f, 3);
    tally.update(0, 3, 30.f, 3);
    tally.update(2, 4, 30.f, 3);
    CHECK((tally.finishOrder() == std::vector<int>{2, 0}));
}

void anUnchangedLapCountAddsNothing()
{
    LapTally tally;
    tally.update(1, 1, 30.f, 3);
    tally.update(1, 1, 30.f, 3);
    CHECK(near(tally.raceTime(1), 30.f));
}

// ---- the attract demo's trigger

void theDemoStartsAfter75IdleSeconds()
{
    AttractIdleClock clock;
    bool started = false;
    for (int frame = 0; frame < 60 * 76 && !started; ++frame)
    {
        started = clock.step(kStep, false, true);
    }
    CHECK(started);
}

void heldInputKeepsTheDemoAway()
{
    AttractIdleClock clock;
    clock.step(74.f, false, true);
    clock.step(kStep, true, true);
    CHECK(!clock.step(2.f, false, true));
}

void excludedPagesWaitForAnAllowedOne()
{
    AttractIdleClock clock;
    CHECK(!clock.step(80.f, false, false));
    CHECK(clock.step(kStep, false, true));
}

void theThreeReplaysTakeTurns()
{
    AttractReplayCycle cycle;
    int played[4];
    for (int &number : played)
    {
        number = cycle.next();
        cycle.advance();
    }
    CHECK(played[0] == 1 && played[1] == 2 && played[2] == 3 && played[3] == 1);
}

// ---- the replayed controls and the start's easing

void controlSlotsReadBackTheBits()
{
    const uint32_t bits = ControlThrottle | ControlRight | ControlClimb | ControlAirbrakeLeft;
    CHECK(controlBitsOf(controlSlotsOf(bits), false) == bits);
}

void aSetBitIsAFullSlot()
{
    CHECK(controlSlotsOf(ControlBrake)[9] == 100.f);
}

void theStartEasesATenthOfTheWay()
{
    const glm::vec3 eased = easedStartPosition(glm::vec3(0.f), glm::vec3(10.f, 0.f, 0.f), 1.f);
    CHECK(near(eased.x, 1.f));
}

void afterThreeSecondsTheRecordedPositionHolds()
{
    const glm::vec3 placed = easedStartPosition(glm::vec3(0.f), glm::vec3(10.f, 0.f, 0.f), 3.f);
    CHECK(near(placed.x, 10.f));
}

// ---- the weapon entries

ProjectileId rocket()
{
    return ProjectileId{4, 3, 1};
}

void anImpactKeepsItsKindAndPlace()
{
    const WeaponEvent e = impactEvent(rocket(), 2, {1.f, 2.f, 3.f});
    CHECK(isImpact(e) && impactKindOf(e) == 2 && impactPositionOf(e) == glm::vec3(1.f, 2.f, 3.f));
}

void aLaunchNamesItsProjectileAndTarget()
{
    const WeaponEvent e = launchEvent(rocket(), 5);
    CHECK(projectileOf(e) == rocket() && launchTargetOf(e) == 5);
}

void aLaunchWithoutTargetSaysNone()
{
    CHECK(launchTargetOf(launchEvent(rocket(), -1)) == -1);
}

void aShieldEntryCarriesItsValueAndKiller()
{
    const WeaponEvent e = shieldEvent(3, 42.5f, 1);
    CHECK(e.craft == 3 && shieldOf(e) == 42.5f && killerOf(e) == 1);
}

void aProjectileStateKeepsItsForwardToTheBytes()
{
    const glm::vec3 forward = glm::normalize(glm::vec3(0.7f, 0.1f, -0.7f));
    const WeaponEvent e = projectileStateEvent(rocket(), {5.f, 6.f, 7.f}, {200.f, 0.f, 0.f}, forward);
    CHECK(glm::length(stateForwardOf(e) - forward) < 0.02f);
}

void aPickupEntryNamesThePickupAndTheCraft()
{
    const WeaponEvent e = pickupTakenEvent(20, 5);
    CHECK(pickupOf(e) == 20 && pickupCraftOf(e) == 5);
}

// a race with a launch in frame 2 and its impact in frame 3, read back
Recording raceWithAShot()
{
    RecordingWriter writer;
    writer.start(twoCraftInfo(), 0.f);
    writer.recordFrame(kStep, {craftAt({0.f, 0.f, 0.f}, {0.f, 0.f, 0.f})});
    writer.recordWeapon(launchEvent(rocket(), -1));
    writer.recordFrame(2.f * kStep, {craftAt({0.f, 0.f, 0.f}, {0.f, 0.f, 0.f})});
    writer.recordWeapon(impactEvent(rocket(), 1, {9.f, 0.f, 0.f}));
    writer.recordFrame(3.f * kStep, {craftAt({0.f, 0.f, 0.f}, {0.f, 0.f, 0.f})});
    writer.stop();
    return readBack(writer.files());
}

void weaponEntriesReadBackInOrder()
{
    const Recording r = raceWithAShot();
    CHECK(r.weapons.size() == 2 && isSignal(r.weapons[0], SignalLaunch) && isImpact(r.weapons[1]));
}

void aWeaponEntryCarriesTheTimeOfTheFrameBefore()
{
    const Recording r = raceWithAShot();
    CHECK(near(r.weapons[1].time, 2.f * kStep, 1e-6f));
}

void weaponEntriesAreCountedInOrder()
{
    const Recording r = raceWithAShot();
    CHECK(r.weapons[0].sequence == 0 && r.weapons[1].sequence == 1);
}

void aReplayedRaceTalliesAsRecorded()
{
    const Recording r = raceWithAShot();
    ShotTally recorded, played;
    recorded.count(launchEvent(rocket(), -1));
    recorded.count(impactEvent(rocket(), 1, {9.f, 0.f, 0.f}));
    for (const WeaponEvent &e : r.weapons)
    {
        played.count(e);
    }
    CHECK(played == recorded);
}

}  // namespace

// ---- the race's recording (logic\race_recording.hpp)

RaceRecording recordingStarted(float simClock)
{
    RaceRecording recording;
    argon::replay::Info lineup;
    lineup.track = 3;
    lineup.pilots[0] = 2;
    recording.start(lineup, simClock, {});
    return recording;
}

void recordingClockWaitsForTheGridIntro()
{
    RaceRecording recording = recordingStarted(0.f);
    recording.tick(true, true);
    recording.tick(false, true);
    CHECK(std::abs(recording.clock() - RaceRecording::kStep) < 1e-6f);
}

void recordingNotesTheFirstStepAfterGo()
{
    RaceRecording recording = recordingStarted(12.5f);
    recording.tick(false, true);
    recording.tick(false, true);
    recording.tick(false, false);
    recording.tick(false, false);
    recording.stop();
    CHECK(recording.lastRaceTiming().goStep == 3 && recording.lastRaceTiming().steps == 4 && recording.lastRaceTiming().simClockAtStart == 12.5f);
}

void recordingWritesAShieldOnlyWhenItChanged()
{
    RaceRecording recording = recordingStarted(0.f);
    recording.recordShield(2, 0.f, 2);    // as started
    recording.recordShield(2, 80.f, 2);
    recording.recordShield(2, 80.f, 2);
    CHECK(recording.weaponsSummary().find(" 1 shield values") != std::string::npos);
}

void recordingStoppedWithoutKeepingLeavesTheLastRace()
{
    RaceRecording recording = recordingStarted(0.f);
    recording.recordFrame({argon::replay::CraftEvent{}});
    recording.stop();
    const argon::replay::RecordingFiles kept = recording.lastRace();
    argon::replay::Info lineup;
    recording.start(lineup, 0.f, {});
    recording.stopWithoutKeeping();
    CHECK(recording.hasLastRace() && recording.lastRace().index == kept.index && !recording.recording());
}

void recordingPastTwentyMinutesIsTooLong()
{
    RaceRecording recording = recordingStarted(0.f);
    for (int step = 0; step <= int(RaceRecording::kLongest * 60.f) + 1; ++step)
    {
        recording.tick(false, false);
    }
    CHECK(recording.pastLongest());
}

int main()
{
    aCraftOnItsReckoningNeedsNoEntry();
    recordingClockWaitsForTheGridIntro();
    recordingNotesTheFirstStepAfterGo();
    recordingWritesAShieldOnlyWhenItChanged();
    recordingStoppedWithoutKeepingLeavesTheLastRace();
    recordingPastTwentyMinutesIsTooLong();
    aCraftOffItsReckoningNeedsAnEntry();
    aVelocityChangeNeedsAnEntry();
    aTurnNeedsAnEntry();
    controlBitsFollowTheSlots();
    framesCloserThanTheIntervalWriteNothing();
    infoReadsBackTheTrackAndPilots();
    eachFrameIsAnIndexEntry();
    aChunkCarriesTheTimeOfTheFrameBefore();
    positionsReadBackExactly();
    rotationsReadBackToTheShortsPrecision();
    aStraightCraftIsWrittenOnlyWhenItDrifts();
    entriesNameTheirCrafts();
    theIndexEndsWithTheMarker();
    aRecordedRacePlaysBackTheSamePoses();
    lapsAddUpToTheRaceTime();
    reachingTheLimitFinishesInOrder();
    anUnchangedLapCountAddsNothing();
    theDemoStartsAfter75IdleSeconds();
    heldInputKeepsTheDemoAway();
    excludedPagesWaitForAnAllowedOne();
    theThreeReplaysTakeTurns();
    controlSlotsReadBackTheBits();
    aSetBitIsAFullSlot();
    theStartEasesATenthOfTheWay();
    afterThreeSecondsTheRecordedPositionHolds();
    anImpactKeepsItsKindAndPlace();
    aLaunchNamesItsProjectileAndTarget();
    aLaunchWithoutTargetSaysNone();
    aShieldEntryCarriesItsValueAndKiller();
    aProjectileStateKeepsItsForwardToTheBytes();
    aPickupEntryNamesThePickupAndTheCraft();
    weaponEntriesReadBackInOrder();
    aWeaponEntryCarriesTheTimeOfTheFrameBefore();
    weaponEntriesAreCountedInOrder();
    aReplayedRaceTalliesAsRecorded();
    std::printf("%d checks, %d failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
