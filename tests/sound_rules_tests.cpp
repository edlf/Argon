// Argon: tests for the race's event sound rules (logic/sound_rules.hpp): f_settings.txt, the hit volumes (0xcabc0),
// which contact plays what (0xc2b50, craft_water 0x61340), the rolloff curve, the score tally (0x95b00) and who hears
// a shield's launch (0x65351), and the weapons' travel loops (logic/travel_loops.hpp, 0xc9c30..0xc9f80). Run with
// test.bat / test.sh.
#include "logic/sound_rules.hpp"
#include "audio/heard_speeds.hpp"
#include "check.hpp"
#include "logic/travel_loops.hpp"

#include <cmath>
#include <cstdio>
#include <string>

namespace
{
using argon::test::checks;
using argon::test::failures;

using argon::GenericSound;
namespace sound = argon::sound;

bool near(float a, float b)
{
    return std::abs(a - b) < 1e-3f;
}

const char *kShippedSettings = "light impact force 20\nheavy impact force 60\nimpact time delay 0.25\nscrape force 1\nscrape trail 0.1\n"
                               "light impact ship 1\nheavy impact ship 2\nlight impact water -1\nheavy impact water -20\n"
                               "max impact force 150\nmax impact water -100\nmax impact ship 3\n";

sound::CraftContact contactOn(int surface, float dot, float speedChange)
{
    sound::CraftContact c;
    c.count = 1;
    c.surface = surface;
    c.normalDotSum = dot;
    c.velocityChange = glm::vec3(speedChange, 0.f, 0.f);
    return c;
}

std::string withoutScrapeTrailFraction()
{
    std::string text = kShippedSettings;
    const std::string fraction = "scrape trail 0.1";
    text.replace(text.find(fraction), fraction.size(), "scrape trail 0");
    return text;
}

void theShippedScrapeTrailFractionLeavesTheShipDefaults()
{
    const sound::ImpactSettings s = sound::parseImpactSettings(kShippedSettings);
    CHECK(near(s.ship.light, 10.f) && near(s.ship.heavy, 60.f) && near(s.ship.max, 100.f));
}

void settingsAfterAWholeScrapeTrailAreRead()
{
    const sound::ImpactSettings s = sound::parseImpactSettings(withoutScrapeTrailFraction());
    CHECK(near(s.ship.light, 1.f) && near(s.ship.heavy, 2.f) && near(s.ship.max, 3.f));
}

void waterSettingsBecomeDownwardSpeeds()
{
    const sound::ImpactSettings s = sound::parseImpactSettings(withoutScrapeTrailFraction());
    CHECK(near(s.water.light, 1.f) && near(s.water.heavy, 20.f) && near(s.water.max, 100.f));
}

void aLineOutOfOrderStopsTheReading()
{
    const sound::ImpactSettings s = sound::parseImpactSettings("heavy impact force 70\nlight impact force 30\n");
    CHECK(near(s.world.light, 20.f) && near(s.world.heavy, 60.f));
}

void aTooLongImpactDelayFallsBack()
{
    const sound::ImpactSettings s = sound::parseImpactSettings("light impact force 20\nheavy impact force 60\nimpact time delay 11\n");
    CHECK(near(s.impactDelay, 0.25f));
}

void missingSettingsKeepTheGameDefaults()
{
    const sound::ImpactSettings s = sound::parseImpactSettings("");
    CHECK(near(s.world.heavy, 60.f) && near(s.ship.light, 10.f) && near(s.impactDelay, 0.25f));
}

void aLightHitAtTheLightForceIsAtMinVol()
{
    CHECK(near(sound::hitVolumeMb(20.f, {20.f, 60.f, 150.f}, {-400.f, -10.f}), -400.f));
}

void aLightHitHalfwayIsHalfwayInVolume()
{
    CHECK(near(sound::hitVolumeMb(40.f, {20.f, 60.f, 150.f}, {-400.f, -10.f}), -205.f));
}

void aHeavyHitStartsAgainFromMinVol()
{
    CHECK(near(sound::hitVolumeMb(60.1f, {20.f, 60.f, 150.f}, {-400.f, -10.f}), -400.f));
}

void aHitBeyondTheMaxIsClampedToSilenceRange()
{
    CHECK(near(sound::hitVolumeMb(1000.f, {20.f, 60.f, 150.f}, {-400.f, 0.f}), 0.f));
}

void noContactStopsTheScrapes()
{
    const sound::ContactSound s = sound::worldContactSound(sound::CraftContact(), sound::ImpactSettings());
    CHECK(s.stopScrapes && s.sound == GenericSound::None);
}

void aGlancingTrackContactScrapesMetal()
{
    const sound::ContactSound s = sound::worldContactSound(contactOn(0, 0.2f, 30.f), sound::ImpactSettings());
    CHECK(s.scrape && s.sound == GenericSound::ScrapeMetal);
}

void aGlancingRoughContactScrapesTheWall()
{
    CHECK(sound::worldContactSound(contactOn(1, 0.2f, 30.f), sound::ImpactSettings()).sound == GenericSound::ScrapeWall);
}

void manyGlancingContactsInAFrameStillScrape()
{
    sound::CraftContact c = contactOn(0, 0.2f, 30.f);
    c.count = 4;
    c.normalDotSum = 4 * 0.2f;
    CHECK(sound::worldContactSound(c, sound::ImpactSettings()).scrape);
}

void anAlmostParallelContactIsSilent()
{
    const sound::ContactSound s = sound::worldContactSound(contactOn(0, 0.05f, 30.f), sound::ImpactSettings());
    CHECK(s.sound == GenericSound::None && !s.stopScrapes);
}

void aHardHeadOnTrackHitIsHeavy()
{
    CHECK(sound::worldContactSound(contactOn(0, 1.f, 70.f), sound::ImpactSettings()).sound == GenericSound::HeavyImpactTrack);
}

void aSoftHeadOnLooseHitIsLight()
{
    CHECK(sound::worldContactSound(contactOn(2, 1.f, 30.f), sound::ImpactSettings()).sound == GenericSound::LightImpactLoose);
}

void aHitBelowTheLightForceIsSilentButEndsScrapes()
{
    const sound::ContactSound s = sound::worldContactSound(contactOn(0, 1.f, 10.f), sound::ImpactSettings());
    CHECK(s.sound == GenericSound::None && s.stopScrapes);
}

void waterContactsOnlyHitLightly()
{
    CHECK(sound::worldContactSound(contactOn(3, 1.f, 100.f), sound::ImpactSettings()).sound == GenericSound::LightImpactWater);
}

void craftPairsHitByVelocityChange()
{
    const sound::ImpactSettings s = sound::parseImpactSettings(kShippedSettings);
    CHECK(sound::shipImpactSound(61.f, s) == GenericSound::HeavyImpactMetal && sound::shipImpactSound(15.f, s) == GenericSound::LightImpactMetal);
}

void aLightCraftTouchIsSilent()
{
    CHECK(sound::shipImpactSound(5.f, sound::parseImpactSettings(kShippedSettings)) == GenericSound::None);
}

void aFastFallIntoWaterIsWaterEnter()
{
    const sound::Splash s = sound::waterSplash(-30.f, sound::parseImpactSettings(kShippedSettings));
    CHECK(s.sound == GenericSound::HeavyImpactWater && s.unlessPlaying == GenericSound::LightImpactWater);
}

void aSlowDipIsALightSplash()
{
    CHECK(sound::waterSplash(-5.f, sound::parseImpactSettings(kShippedSettings)).sound == GenericSound::LightImpactWater);
}

void risingOutOfWaterIsSilent()
{
    CHECK(sound::waterSplash(2.f, sound::parseImpactSettings(kShippedSettings)).sound == GenericSound::None);
}

void loopsOneLeavesMinVolAtFull()
{
    sound::VolumeRange volume;
    sound::scanLoopsInto(1, volume);
    CHECK(volume.minMb == 0.f);
}

void aNegativeMinVolLeavesMaxVolMinusOne()
{
    sound::VolumeRange volume;
    sound::scanMinVolumeInto(-1000, volume);
    CHECK(volume.minMb == -1000.f && volume.maxMb == -1.f);
}

void aLoopingScrapeAtTheLightForcePlaysAtFull()
{
    sound::VolumeRange volume;
    sound::scanLoopsInto(1, volume);
    CHECK(near(sound::hitVolumeMb(22.f, {20.f, 60.f, 150.f}, volume), 0.f));
}

void rolloffIsFullInsideMinDist()
{
    CHECK(near(sound::rolloffGain(10.f, 20.f, 200.f), 1.f));
}

void rolloffHoldsFullOverTheFirstStep()
{
    CHECK(near(sound::rolloffGain(56.f, 20.f, 200.f), 1.f));
}

void rolloffHalvesAtTheSecondStep()
{
    CHECK(near(sound::rolloffGain(92.f, 20.f, 200.f), 0.5f));
}

void rolloffRunsBetweenTwoPoints()
{
    CHECK(near(sound::rolloffGain(110.f, 20.f, 200.f), 0.375f));
}

void rolloffIsSilentAtMaxDist()
{
    CHECK(near(sound::rolloffGain(200.f, 20.f, 200.f), 0.f));
}

void aZeroWidthRangeKeepsFullVolumeFarAway()
{
    CHECK(near(sound::rolloffGain(2800.f, 1000.f, 1000.f), 1.f));
}

void theTallyClimbsByAGrowingStep()
{
    sound::ScoreTally t;
    t.start(100000);
    t.step();
    t.step();
    CHECK(t.shown() == 0x2e1 + 0x2e1 + 0x18);
}

void theTallyStopsAtTheScore()
{
    sound::ScoreTally t;
    t.start(1000);
    t.step();
    const bool reached = t.step();
    CHECK(reached && t.shown() == 1000 && !t.counting());
}

void theChingRisesWithTheStep()
{
    sound::ScoreTally t;
    t.start(100000);
    t.step();
    CHECK(near(t.chingPitch(), (22050.f + 2.f * float(0x2e1 + 0x18)) / 22050.f));
}

void thePlayersShieldLaunchSounds()
{
    CHECK(sound::shieldLaunchSounds(0, false));
}

void anotherCraftsShieldLaunchIsSilent()
{
    CHECK(!sound::shieldLaunchSounds(3, false));
}

void everyShieldLaunchSoundsInSplitScreen()
{
    CHECK(sound::shieldLaunchSounds(1, true));
}
using sound::TravelKey;
using sound::TravelLaunch;
using sound::TravelLoops;

TravelLaunch launchOf(int projectile, int weaponClass = 0)
{
    return TravelLaunch{TravelKey{0, 1, projectile}, weaponClass};
}

void eachClassHasItsTravelSound()
{
    CHECK(sound::travelSound(0) == argon::ShipSound::TravelProjectile && sound::travelSound(1) == argon::ShipSound::TravelLaser &&
          sound::travelSound(2) == argon::ShipSound::TravelRadial);
}

void anyOtherClassTravelsAsTheSpecial()
{
    CHECK(sound::travelSound(5) == argon::ShipSound::TravelSpecial);
}

void theRadialsDistancesGrowFromTheRadius()
{
    const sound::TravelDistances d = sound::radialTravelDistances(100.f, 20.f, 200.f);
    CHECK(near(d.minDist, 300.f) && near(d.maxDist, 780.f));
}

void aQueuedLoopWaitsHalfASecond()
{
    TravelLoops loops;
    loops.queue(launchOf(0), 10.f);
    CHECK(loops.takeDue(10.49f).empty());
}

void aQueuedLoopStartsOnceDue()
{
    TravelLoops loops;
    loops.queue(launchOf(0), 10.f);
    const std::vector<TravelLaunch> due = loops.takeDue(10.5f);
    CHECK(due.size() == 1 && due[0].key == launchOf(0).key && loops.takeDue(11.f).empty());
}

void anEleventhLaunchStartsTheEarliestAtOnce()
{
    TravelLoops loops;
    for (int k = 0; k < TravelLoops::kSlots; ++k)
    {
        loops.queue(launchOf(k), float(k));
    }
    const auto early = loops.queue(launchOf(10), 9.5f);
    CHECK(early && early->key == launchOf(0).key);
}

void aStoppedProjectileNeverStartsItsLoop()
{
    TravelLoops loops;
    loops.queue(launchOf(0), 0.f);
    loops.stop(launchOf(0).key);
    CHECK(loops.takeDue(1.f).empty());
}

void aStartedLoopIsFoundByItsKey()
{
    TravelLoops loops;
    loops.holdVoice(launchOf(2).key, 7, 0.f);
    CHECK(loops.voicesOf(launchOf(2).key) == std::vector<int>{7} && loops.voicesOf(launchOf(3).key).empty());
}

void stoppingAProjectileGivesItsVoice()
{
    TravelLoops loops;
    loops.holdVoice(launchOf(2).key, 7, 0.f);
    CHECK(loops.stop(launchOf(2).key) == std::vector<int>{7} && loops.voicesOf(launchOf(2).key).empty());
}

void anEleventhVoiceReplacesTheEarliestStarted()
{
    TravelLoops loops;
    for (int k = 0; k < TravelLoops::kSlots; ++k)
    {
        loops.holdVoice(launchOf(k).key, 100 + k, float(10 - k));
    }
    CHECK(loops.holdVoice(launchOf(10).key, 200, 20.f) == 109);
}

void aLoopThatDidNotStartLeavesItsSlotFree()
{
    TravelLoops loops;
    loops.holdVoice(launchOf(0).key, TravelLoops::kNoVoice, 0.f);
    for (int k = 1; k < TravelLoops::kSlots; ++k)
    {
        loops.holdVoice(launchOf(k).key, k, 1.f);
    }
    CHECK(loops.holdVoice(launchOf(10).key, 50, 2.f) == TravelLoops::kNoVoice);
}

void stoppingAllGivesEveryVoiceAndEmptiesTheQueue()
{
    TravelLoops loops;
    loops.holdVoice(launchOf(0).key, 3, 0.f);
    loops.holdVoice(launchOf(1).key, 4, 0.f);
    loops.queue(launchOf(2), 0.f);
    CHECK(loops.stopAll().size() == 2 && loops.takeDue(1.f).empty());
}

void theHeardSpeedFollowsByAtMostOnePerFrame()
{
    HeardSpeeds heard;
    heard.follow(2, -30.f);
    heard.follow(2, -30.f);
    CHECK(heard.of(2) == 2.f);
}

void aRestartedCraftIsHeardFromStanding()
{
    HeardSpeeds heard;
    heard.follow(0, 5.f);
    heard.restart(0);
    CHECK(heard.of(0) == 0.f);
}

}  // namespace

int main()
{
    theShippedScrapeTrailFractionLeavesTheShipDefaults();
    settingsAfterAWholeScrapeTrailAreRead();
    aLineOutOfOrderStopsTheReading();
    aTooLongImpactDelayFallsBack();
    waterSettingsBecomeDownwardSpeeds();
    missingSettingsKeepTheGameDefaults();
    aLightHitAtTheLightForceIsAtMinVol();
    aLightHitHalfwayIsHalfwayInVolume();
    aHeavyHitStartsAgainFromMinVol();
    aHitBeyondTheMaxIsClampedToSilenceRange();
    noContactStopsTheScrapes();
    aGlancingTrackContactScrapesMetal();
    aGlancingRoughContactScrapesTheWall();
    manyGlancingContactsInAFrameStillScrape();
    anAlmostParallelContactIsSilent();
    aHardHeadOnTrackHitIsHeavy();
    aSoftHeadOnLooseHitIsLight();
    aHitBelowTheLightForceIsSilentButEndsScrapes();
    waterContactsOnlyHitLightly();
    craftPairsHitByVelocityChange();
    aLightCraftTouchIsSilent();
    aFastFallIntoWaterIsWaterEnter();
    aSlowDipIsALightSplash();
    risingOutOfWaterIsSilent();
    loopsOneLeavesMinVolAtFull();
    aNegativeMinVolLeavesMaxVolMinusOne();
    aLoopingScrapeAtTheLightForcePlaysAtFull();
    rolloffIsFullInsideMinDist();
    rolloffHoldsFullOverTheFirstStep();
    rolloffHalvesAtTheSecondStep();
    rolloffRunsBetweenTwoPoints();
    rolloffIsSilentAtMaxDist();
    aZeroWidthRangeKeepsFullVolumeFarAway();
    theTallyClimbsByAGrowingStep();
    theTallyStopsAtTheScore();
    theChingRisesWithTheStep();
    thePlayersShieldLaunchSounds();
    anotherCraftsShieldLaunchIsSilent();
    everyShieldLaunchSoundsInSplitScreen();
    eachClassHasItsTravelSound();
    anyOtherClassTravelsAsTheSpecial();
    theRadialsDistancesGrowFromTheRadius();
    aQueuedLoopWaitsHalfASecond();
    aQueuedLoopStartsOnceDue();
    anEleventhLaunchStartsTheEarliestAtOnce();
    aStoppedProjectileNeverStartsItsLoop();
    aStartedLoopIsFoundByItsKey();
    stoppingAProjectileGivesItsVoice();
    anEleventhVoiceReplacesTheEarliestStarted();
    aLoopThatDidNotStartLeavesItsSlotFree();
    stoppingAllGivesEveryVoiceAndEmptiesTheQueue();
    theHeardSpeedFollowsByAtMostOnePerFrame();
    aRestartedCraftIsHeardFromStanding();
    std::printf("%d of %d checks passed\n", checks - failures, checks);
    return failures == 0 ? 0 : 1;
}
