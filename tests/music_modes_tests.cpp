// Argon: tests for the game's music modes (audio/music_modes.hpp), the race events that change them
// (logic/music_cues.hpp) and the race end's music fades (audio/music_fade.hpp). No window, GPU or sound card: run with test.bat / test.sh (or ctest in build).
#include "audio/music_fade.hpp"
#include "check.hpp"
#include "audio/music_modes.hpp"
#include "logic/music_cues.hpp"

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace
{
using argon::test::checks;
using argon::test::failures;

const size_t kIntro = 0, kOnStart = 1, kSecondLap = 2, kSegment1 = 3;

argon::MusicFolder folderOf(int segments, bool stingers = true, bool secondLap = false)
{
    argon::MusicFolder folder;
    if (stingers)
    {
        folder.intro = "intro.wav";
        folder.onStart = "on_start.wav";
    }
    if (secondLap)
    {
        folder.secondLap = "on_start_second_lap.wav";
    }
    for (int n = 1; n <= segments; ++n)
    {
        folder.segments.push_back("t_music" + std::to_string(n) + ".wav");
    }
    return folder;
}

// the songs the stream would play, one after another
std::vector<size_t> playSongs(argon::MusicModes &modes, int count)
{
    std::vector<size_t> songs;
    for (int i = 0; i < count; ++i)
    {
        songs.push_back(modes.next());
        modes.started(songs.back());
    }
    return songs;
}

argon::MusicModes raceAfterIntro(const argon::MusicFolder &folder)
{
    argon::MusicModes modes(folder);
    modes.restart(folder.playlist().size());
    modes.set(argon::MusicModes::Intro);
    modes.started(modes.next());
    return modes;
}

void testRaceLoadPlaysTheIntroUntilTheStart()
{
    argon::MusicModes modes = raceAfterIntro(folderOf(5));
    CHECK(playSongs(modes, 2) == (std::vector<size_t>{kIntro, kIntro}));
}

void testStartPlaysOnStartOnceThenTheFirstSegment()
{
    argon::MusicModes modes = raceAfterIntro(folderOf(5));
    modes.set(argon::MusicModes::OnStart);
    CHECK(playSongs(modes, 2) == (std::vector<size_t>{kOnStart, kSegment1}));
}

void testEachSegmentPlaysTwiceThenTheNext()
{
    argon::MusicModes modes(folderOf(5));
    CHECK(playSongs(modes, 4) == (std::vector<size_t>{kSegment1, kSegment1, kSegment1 + 1, kSegment1 + 1}));
}

void testFiveSegmentsCycleThroughTheFirstFour()
{
    argon::MusicModes modes(folderOf(5));
    const std::vector<size_t> songs = playSongs(modes, 10);
    CHECK(songs[6] == kSegment1 + 3 && songs[8] == kSegment1);
}

void testFourSegmentsRepeatTheThirdForTheFourthMode()
{
    argon::MusicModes modes(folderOf(4));
    const std::vector<size_t> songs = playSongs(modes, 8);
    CHECK(songs[4] == kSegment1 + 2 && songs[6] == kSegment1 + 2);
}

void testFinalePlaysTheLastSegment()
{
    argon::MusicModes modes(folderOf(5));
    playSongs(modes, 1);
    modes.playFinale();
    CHECK(modes.next() == kSegment1 + 4);
}

void testViewReplayStartsOverOnOnStartAfterTheFinale()
{
    argon::MusicModes modes(folderOf(5));
    playSongs(modes, 1);
    modes.playFinale();
    playSongs(modes, 1);
    modes.startOver(argon::MusicModes::OnStart);
    CHECK(playSongs(modes, 3) == (std::vector<size_t>{kOnStart, kSegment1, kSegment1}));
}

void testStartingOverOnTheIntroPlaysItUntilTheStart()
{
    argon::MusicModes modes(folderOf(5));
    playSongs(modes, 3);
    modes.startOver(argon::MusicModes::Intro);
    CHECK(playSongs(modes, 2) == (std::vector<size_t>{kIntro, kIntro}));
}

void testStepUpMovesOnWithoutTheRepeat()
{
    argon::MusicModes modes(folderOf(5));
    playSongs(modes, 1);
    modes.stepUp();
    CHECK(modes.next() == kSegment1 + 1);
}

void testStepUpStopsAtTheFifthSegmentMode()
{
    argon::MusicModes modes(folderOf(6));
    for (int i = 0; i < 8; ++i)
    {
        modes.stepUp();
    }
    CHECK(modes.mode() == 7);
}

void testStepDownStopsAtTheFirstSegment()
{
    argon::MusicModes modes(folderOf(5));
    modes.stepDown();
    CHECK(modes.mode() == argon::MusicModes::FirstSegment);
}

void testSecondLapWithoutItsFileChangesNothing()
{
    argon::MusicModes modes(folderOf(5));
    modes.set(argon::MusicModes::SecondLap);
    CHECK(modes.mode() == argon::MusicModes::FirstSegment);
}

void testSecondLapReturnsToTheModeItInterrupted()
{
    argon::MusicModes modes(folderOf(4, true, true));
    modes.stepUp();
    modes.set(argon::MusicModes::SecondLap);
    CHECK(playSongs(modes, 2) == (std::vector<size_t>{kSecondLap, kSegment1 + 1}));
}

void testMissingOnStartGoesToTheFirstSegment()
{
    argon::MusicModes modes(folderOf(5, false));
    modes.set(argon::MusicModes::Intro);
    CHECK(modes.next() == kSegment1);
}

void testFinalLapStepsUpInAThreeLapRace()
{
    CHECK(argon::music::finalLapStarts(3, 3) && !argon::music::secondLapStarts(3, 3));
}

void testLapTwoOfTwoIsTheFinalLapNotTheSecondLapStinger()
{
    CHECK(argon::music::finalLapStarts(2, 2) && !argon::music::secondLapStarts(2, 2));
}

void testLapTwoOfThreePlaysTheStinger()
{
    CHECK(argon::music::secondLapStarts(2, 3));
}

void testLeadChanges()
{
    CHECK(argon::music::takesTheLead(2, 1) && argon::music::losesTheLead(1, 3) && !argon::music::takesTheLead(1, 1));
}
// the race end's fades at the default Music Volume (step 4)
const float kSettingMb = -400.f;

bool near(float a, float b)
{
    return std::fabs(a - b) < 0.5f;
}

void testFadeOutHoldsTheSettingUntilTheRampReachesIt()
{
    CHECK(near(argon::musicFadeMb(argon::MusicFadeShape::Out, 0.02, kSettingMb), kSettingMb));
}

void testFadeOutFallsTenThousandMillibelsASecond()
{
    CHECK(near(argon::musicFadeMb(argon::MusicFadeShape::Out, 0.25, kSettingMb), -2500.f));
}

void testFadeOutStopsAtTheFloor()
{
    CHECK(near(argon::musicFadeMb(argon::MusicFadeShape::Out, 1.5, kSettingMb), -9600.f));
}

void testFadeOutAtTheSilentSettingRisesToTheFloor()
{
    CHECK(near(argon::musicFadeMb(argon::MusicFadeShape::Out, 0.0, -10000.f), -10000.f) &&
          near(argon::musicFadeMb(argon::MusicFadeShape::Out, 0.99, -10000.f), -9600.f));
}

void testDipBottomsOutAtHalfASecond()
{
    CHECK(near(argon::musicFadeMb(argon::MusicFadeShape::Dip, 0.5, kSettingMb), -5000.f));
}

void testDipClimbsBackFromMinusFourThousand()
{
    CHECK(near(argon::musicFadeMb(argon::MusicFadeShape::Dip, 0.6, kSettingMb), -3000.f));
}

void testDipIsBackAtTheSettingBeforeTheSecondEnds()
{
    CHECK(near(argon::musicFadeMb(argon::MusicFadeShape::Dip, 0.9, kSettingMb), kSettingMb));
}

void testDipPastTheSecondIsTheSetting()
{
    CHECK(near(argon::musicFadeMb(argon::MusicFadeShape::Dip, 1.2, -1600.f), -1600.f));
}

void testFadeGainIsOneAtTheSettingAndATenthTwentyDecibelsUnder()
{
    CHECK(std::fabs(argon::musicFadeGain(kSettingMb, kSettingMb) - 1.f) < 1e-6f && std::fabs(argon::musicFadeGain(-2400.f, kSettingMb) - 0.1f) < 1e-6f);
}

void testFadeIsOverOnlyAfterItsSecond()
{
    argon::MusicFade fade;
    fade.start(argon::MusicFadeShape::Out, 10.0);
    CHECK(!fade.over(11.0) && fade.over(11.01));
}

void testFadeLevelCountsFromItsStart()
{
    argon::MusicFade fade;
    fade.start(argon::MusicFadeShape::Dip, 20.0);
    CHECK(near(fade.levelMb(20.25, kSettingMb), -2500.f));
}

void theGridsPlaceIsNoLeadChange()
{
    argon::music::LeadWatch watch;
    CHECK(watch.see(1) == argon::music::LeadCue::None);
}

void reachingFirstTakesTheLead()
{
    argon::music::LeadWatch watch;
    watch.see(3);
    CHECK(watch.see(1) == argon::music::LeadCue::TookTheLead);
}

void leavingFirstLosesTheLead()
{
    argon::music::LeadWatch watch;
    watch.see(1);
    CHECK(watch.see(2) == argon::music::LeadCue::LostTheLead);
}

void aResetMakesTheNextPlaceTheGrids()
{
    argon::music::LeadWatch watch;
    watch.see(3);
    watch.reset();
    CHECK(watch.see(1) == argon::music::LeadCue::None);
}

}  // namespace

int main()
{
    testRaceLoadPlaysTheIntroUntilTheStart();
    testStartPlaysOnStartOnceThenTheFirstSegment();
    testEachSegmentPlaysTwiceThenTheNext();
    testFiveSegmentsCycleThroughTheFirstFour();
    testFourSegmentsRepeatTheThirdForTheFourthMode();
    testFinalePlaysTheLastSegment();
    testViewReplayStartsOverOnOnStartAfterTheFinale();
    testStartingOverOnTheIntroPlaysItUntilTheStart();
    testStepUpMovesOnWithoutTheRepeat();
    testStepUpStopsAtTheFifthSegmentMode();
    testStepDownStopsAtTheFirstSegment();
    testSecondLapWithoutItsFileChangesNothing();
    testSecondLapReturnsToTheModeItInterrupted();
    testMissingOnStartGoesToTheFirstSegment();
    testFinalLapStepsUpInAThreeLapRace();
    testLapTwoOfTwoIsTheFinalLapNotTheSecondLapStinger();
    testLapTwoOfThreePlaysTheStinger();
    testLeadChanges();
    testFadeOutHoldsTheSettingUntilTheRampReachesIt();
    testFadeOutFallsTenThousandMillibelsASecond();
    testFadeOutStopsAtTheFloor();
    testFadeOutAtTheSilentSettingRisesToTheFloor();
    testDipBottomsOutAtHalfASecond();
    testDipClimbsBackFromMinusFourThousand();
    testDipIsBackAtTheSettingBeforeTheSecondEnds();
    testDipPastTheSecondIsTheSetting();
    testFadeGainIsOneAtTheSettingAndATenthTwentyDecibelsUnder();
    testFadeIsOverOnlyAfterItsSecond();
    testFadeLevelCountsFromItsStart();
    theGridsPlaceIsNoLeadChange();
    reachingFirstTakesTheLead();
    leavingFirstLosesTheLead();
    aResetMakesTheNextPlaceTheGrids();
    std::printf("%d checks, %d failed\n", checks, failures);
    return failures ? 1 : 0;
}
