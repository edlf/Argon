// Argon: tests for the pickups (logic/pickups.hpp: pickups_collide 0x8fcd0, the loader's lift 0x78380, the
// once-per-lap flags 0x52c860) and the particles (logic/particles.hpp: 0x88750 bursts, 0xbc640 step, 0xbcb60 cap).
// No window or GPU: run with test.bat / test.sh (or ctest in build).
#include "logic/particles.hpp"
#include "logic/pickups.hpp"
#include "check.hpp"

#include <cmath>
#include <cstdio>
#include <vector>

namespace
{
using argon::test::checks;
using argon::test::failures;

using namespace argon;

const float kBlueLift = 3.84f;

Pickups onePickup(const std::string &type)
{
    Pickups pickups;
    pickups.place({Pickup{type, {0.f, 0.f, 0.f}}});
    return pickups;
}

glm::vec3 at(const Pickups &pickups)
{
    return pickups.position(0);
}

Particle particleBornAt(float born, float life)
{
    Particle particle{};
    particle.ship = -1;
    particle.born = born;
    particle.dur = life;
    return particle;
}

void testAPickupFloatsAboveItsSpotByItsTypesLift()
{
    CHECK(std::abs(onePickup("POWERUPBLUE").position(0).y - kBlueLift) < 1e-5f);
}

void testTheNamesMapToTheGamesTypes()
{
    CHECK(pickupTypeOf("MEGAUP") == PickupType::MegaUp && pickupTypeOf("POWERCHIP") == PickupType::PowerChip &&
          pickupTypeOf("POWERUPRED") == PickupType::Red && pickupTypeOf("SOMETHING") == PickupType::Blue);
}

void testThePlayerWithinReachTakesThePickup()
{
    Pickups pickups = onePickup("POWERUPRED");
    const std::vector<PickupChange> changes = pickups.stepPlayer(at(pickups) + glm::vec3(13.f, 0.f, 0.f), 0.f);
    CHECK(changes.size() == 1 && changes[0].kind == PickupChange::Kind::Taken && changes[0].type == PickupType::Red);
}

void testThePlayerOutOfReachTakesNothing()
{
    Pickups pickups = onePickup("POWERUPRED");
    CHECK(pickups.stepPlayer(at(pickups) + glm::vec3(10.f, 0.f, 10.f), 0.f).empty());
}

void testATakenPickupComesBackAfterFourTenths()
{
    Pickups pickups = onePickup("POWERUPRED");
    pickups.stepPlayer(at(pickups), 1.f);
    const std::vector<PickupChange> changes = pickups.stepPlayer(glm::vec3(1000.f), 1.41f);
    CHECK(changes.size() == 1 && changes[0].kind == PickupChange::Kind::Back && pickups.onTrack(0));
}

void testATakenPickupIsStillGoneBeforeFourTenths()
{
    Pickups pickups = onePickup("POWERUPRED");
    pickups.stepPlayer(at(pickups), 1.f);
    CHECK(pickups.stepPlayer(glm::vec3(1000.f), 1.39f).empty() && !pickups.onTrack(0));
}

void testThePlayerTakesAPickupOncePerLap()
{
    Pickups pickups = onePickup("POWERUPRED");
    pickups.stepPlayer(at(pickups), 0.f);
    pickups.stepPlayer(at(pickups), 1.f);   // back on the track
    CHECK(pickups.stepPlayer(at(pickups), 1.1f).empty() && pickups.takenThisLapCount() == 1);
}

void testANewLapLetsThePlayerTakeItAgain()
{
    Pickups pickups = onePickup("POWERUPRED");
    pickups.stepPlayer(at(pickups), 0.f);
    pickups.stepPlayer(at(pickups), 1.f);
    pickups.startLap(1);
    CHECK(pickups.stepPlayer(at(pickups), 1.1f).size() == 1);
}

void testAPowerChipNeverComesBack()
{
    Pickups pickups = onePickup("POWERCHIP");
    pickups.stepPlayer(at(pickups), 0.f);
    CHECK(pickups.stepPlayer(glm::vec3(1000.f), 100.f).empty() && !pickups.onTrack(0));
}

void testTheAiLeavesPowerChips()
{
    Pickups pickups = onePickup("POWERCHIP");
    CHECK(pickups.takeByAi(at(pickups), 0.f).empty());
}

void testTheAiHasNoOncePerLapRule()
{
    Pickups pickups = onePickup("MEGAUP");
    pickups.takeByAi(at(pickups), 0.f);
    pickups.stepPlayer(glm::vec3(1000.f), 1.f);   // back on the track
    CHECK(pickups.takeByAi(at(pickups), 1.1f).size() == 1 && pickups.takenThisLapCount() == 0);
}

void testAReplayedIndexTheTrackHasNotIsIgnored()
{
    Pickups pickups = onePickup("POWERUPRED");
    CHECK(!pickups.takeReplayed(5, 0.f) && !pickups.bringBack(-1));
}

void testAReplayedTakeAndReturnMoveThePickup()
{
    Pickups pickups = onePickup("POWERUPRED");
    pickups.takeReplayed(0, 0.f);
    const bool goneAfterTake = !pickups.onTrack(0);
    pickups.bringBack(0);
    CHECK(goneAfterTake && pickups.onTrack(0));
}

void testABurstThrows450Particles()
{
    Particles particles;
    ViewerRand rand;
    particles.burst(ParticleBurst{glm::vec3(0.f), -1, 20.f, 0xff3030ffu}, 0.f, rand);
    CHECK(particles.size() == 75 * 2 * 3);
}

void testParticlesStopAtTheCap()
{
    Particles particles;
    for (int i = 0; i < Particles::kMax + 10; ++i)
    {
        particles.add(particleBornAt(0.f, 1.f));
    }
    CHECK(particles.size() == size_t(Particles::kMax) && !particles.hasRoom());
}

void testAParticleGoesAtTheEndOfItsLife()
{
    Particles particles;
    particles.add(particleBornAt(0.f, 1.f));
    particles.add(particleBornAt(0.5f, 1.f));
    particles.step({}, 1.f);
    CHECK(particles.size() == 1);
}

void testBubblesStopAtTheirSystemsThousand()
{
    Particles particles;
    ViewerRand rand;
    for (int frame = 0; frame < 40; ++frame)
    {
        particles.emitBubbles(glm::vec3(0.f), 0.f, rand);
    }
    CHECK(particles.size() == 1000);
}
}  // namespace

int main()
{
    testAPickupFloatsAboveItsSpotByItsTypesLift();
    testTheNamesMapToTheGamesTypes();
    testThePlayerWithinReachTakesThePickup();
    testThePlayerOutOfReachTakesNothing();
    testATakenPickupComesBackAfterFourTenths();
    testATakenPickupIsStillGoneBeforeFourTenths();
    testThePlayerTakesAPickupOncePerLap();
    testANewLapLetsThePlayerTakeItAgain();
    testAPowerChipNeverComesBack();
    testTheAiLeavesPowerChips();
    testTheAiHasNoOncePerLapRule();
    testAReplayedIndexTheTrackHasNotIsIgnored();
    testAReplayedTakeAndReturnMoveThePickup();
    testABurstThrows450Particles();
    testParticlesStopAtTheCap();
    testAParticleGoesAtTheEndOfItsLife();
    testBubblesStopAtTheirSystemsThousand();
    std::printf("pickups_tests: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
