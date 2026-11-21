// Argon: tests for projectiles, beams and volleys in flight (logic/shot_flight.hpp: 0x64e20, 0x66d30, 0x66e60, 0x67753,
// 0x675f2, 0x683b0, 0x68620, 0x69e30), on a race of crafts (logic/race_crafts.hpp) and a world of one plane.
// No window or GPU: run with test.bat / test.sh (or ctest in build).
#include "logic/shot_flight.hpp"
#include "check.hpp"

#include <cmath>
#include <cstdio>
#include <utility>
#include <vector>

namespace
{
using argon::test::checks;
using argon::test::failures;

using namespace argon::weapons;

// crafts in slots 0..n - 1 at the given places
class PlacedCrafts final : public RaceCrafts
{
  public:
    explicit PlacedCrafts(std::vector<glm::vec3> places) : places_(std::move(places))
    {
    }
    int racerCount() const override
    {
        return int(places_.size());
    }
    int slotOf(int racer) const override
    {
        return racer;
    }
    bool flies(int slot) const override
    {
        return slot >= 0 && slot < racerCount();
    }
    glm::vec3 position(int slot) const override
    {
        return places_[size_t(slot)];
    }
    glm::mat4 frame(int) const override
    {
        return glm::mat4(1.f);
    }
    bool targetable(int) const override
    {
        return true;
    }

  private:
    std::vector<glm::vec3> places_;
};

// one plane through `point` facing `normal`
class PlaneWorld final : public WorldRay
{
  public:
    PlaneWorld(glm::vec3 point, glm::vec3 normal) : point_(point), normal_(normal)
    {
    }
    std::optional<WorldContact> along(const glm::vec3 &from, const glm::vec3 &to) const override
    {
        const float a = glm::dot(from - point_, normal_), b = glm::dot(to - point_, normal_);
        if ((a > 0.f) == (b > 0.f))
        {
            return std::nullopt;
        }
        return WorldContact{from + (to - from) * (a / (a - b)), normal_};
    }

  private:
    glm::vec3 point_, normal_;
};

bool near(const glm::vec3 &a, const glm::vec3 &b)
{
    return glm::length(a - b) < 1e-3f;
}

Shot shotAt(const WeaponRec &weaponType, glm::vec3 pos)
{
    Shot shot;
    shot.weaponType = &weaponType;
    shot.pos = pos;
    return shot;
}

void testAShotLeavesBackAlongItsBoneAtTheCraftsSpeedShare()
{
    WeaponRec weaponType;
    weaponType.back = 2.f;
    weaponType.speedPct = 150.f;
    weaponType.speedAdd = 10.f;
    glm::mat4 bone(1.f);
    bone[3] = glm::vec4(0.f, 0.f, 5.f, 1.f);
    const Shot shot = makeShot(ShotLaunch{1, 0, &weaponType}, ShotStart{bone, 2, 3.f}, ShooterMotion{100.f, glm::vec3(0.f), glm::vec3(0.f, 0.f, 1.f)});
    CHECK(near(shot.pos, glm::vec3(0.f, 0.f, 3.f)) && std::abs(shot.speed - 160.f) < 1e-3f && shot.index == 2 && shot.born == 3.f);
}

void testTheCraftFirstAlongThePathIsHit()
{
    const PlacedCrafts crafts({glm::vec3(0.f), glm::vec3(0.f, 0.f, 30.f), glm::vec3(1.f, 0.f, 20.f)});
    CHECK(craftHitAlong(0, Segment{glm::vec3(0.f), glm::vec3(0.f, 0.f, 50.f)}, 4.f, crafts) == 2);
}

void testACraftWideOfThePathIsNotHit()
{
    const PlacedCrafts crafts({glm::vec3(0.f), glm::vec3(10.f, 0.f, 20.f)});
    CHECK(craftHitAlong(0, Segment{glm::vec3(0.f), glm::vec3(0.f, 0.f, 50.f)}, 4.f, crafts) == -1);
}

void testTheShooterIsNeverNearItsOwnShot()
{
    const PlacedCrafts crafts({glm::vec3(0.f), glm::vec3(0.f, 0.f, 200.f), glm::vec3(0.f, 0.f, 400.f)});
    CHECK(craftsNear(glm::vec3(0.f), 0, crafts) == std::vector<int>({1}));
}

void testAShotRunningIntoAWallHitsIt()
{
    WeaponRec weaponType;
    Shot shot = shotAt(weaponType, glm::vec3(0.f, 0.f, 12.f));
    const PlaneWorld wall(glm::vec3(0.f, 0.f, 10.f), glm::vec3(0.f, 0.f, -1.f));
    const WorldHit hit = worldHitAlong(shot, glm::vec3(0.f, 0.f, 8.f), false, wall);
    CHECK(hit.hit && near(hit.point, glm::vec3(0.f, 0.f, 10.f)));
}

void testASideHuggingShotSlidesOffTheWall()
{
    WeaponRec weaponType;
    weaponType.hugSide = true;
    weaponType.hugDist = 3.f;
    Shot shot = shotAt(weaponType, glm::vec3(0.f, 0.f, 12.f));
    const PlaneWorld wall(glm::vec3(0.f, 0.f, 10.f), glm::vec3(0.f, 0.f, -1.f));
    const WorldHit hit = worldHitAlong(shot, glm::vec3(0.f, 0.f, 8.f), false, wall);
    CHECK(!hit.hit && hit.slid && near(shot.pos, glm::vec3(0.f, 0.f, 7.f)));
}

void testAShotHuggingBelowSitsItsDistanceOffTheFloor()
{
    WeaponRec weaponType;
    weaponType.hugBelow = true;
    weaponType.hugDist = 2.f;
    Shot shot = shotAt(weaponType, glm::vec3(0.f, 1.f, 0.f));
    shot.fall = 5.f;
    const PlaneWorld floor(glm::vec3(0.f), glm::vec3(0.f, 1.f, 0.f));
    const std::optional<glm::vec3> hugged = hugSurfaces(shot, WorldHit{}, floor);
    CHECK(hugged && near(shot.pos, glm::vec3(0.f, 2.f, 0.f)) && shot.fall == 0.f);
}

void testABounceReflectsTheShotAndLiftsItOffTheSurface()
{
    WeaponRec weaponType;
    weaponType.bounces = true;
    weaponType.maxBounces = 1;
    Shot shot = shotAt(weaponType, glm::vec3(0.f));
    shot.fwd = glm::normalize(glm::vec3(0.f, -1.f, 1.f));
    WorldHit floor;
    floor.hit = true;
    floor.normal = glm::vec3(0.f, 1.f, 0.f);
    const bool could = canBounce(shot);
    bounceOff(shot, floor);
    CHECK(could && !canBounce(shot) && shot.fwd.y > 0.f && near(shot.pos, glm::vec3(0.f, 0.4f, 0.f)));
}

void testHomingFindsNoTargetOutOfRange()
{
    WeaponRec weaponType;
    weaponType.lockRange = 50.f;
    const Shot shot = shotAt(weaponType, glm::vec3(0.f));
    const PlacedCrafts crafts({glm::vec3(0.f), glm::vec3(0.f, 0.f, 500.f)});
    argon::ViewerRand rand;
    int found = -1;
    for (int frame = 0; frame < 100; ++frame)
    {
        found = std::max(found, homingTarget(shot, crafts, rand));
    }
    CHECK(found == -1);
}

void testHomingFindsACraftInRangeSooner()
{
    WeaponRec weaponType;
    weaponType.lockRange = 50.f;
    const Shot shot = shotAt(weaponType, glm::vec3(0.f));
    const PlacedCrafts crafts({glm::vec3(0.f), glm::vec3(0.f, 0.f, 20.f)});
    argon::ViewerRand rand;
    int found = -1;
    for (int frame = 0; frame < 100 && found < 0; ++frame)
    {
        found = homingTarget(shot, crafts, rand);
    }
    CHECK(found == 1);
}

void testTheBeamReaches1000mWithoutATarget()
{
    WeaponRec weaponType;
    const Shot shot = shotAt(weaponType, glm::vec3(0.f));
    CHECK(near(beamAimPoint(shot, glm::mat4(1.f), std::nullopt), glm::vec3(0.f, 0.f, 1000.f)));
}

void testAVolleysProjectilesLeaveOneStaggerApart()
{
    WeaponRec weaponType;
    weaponType.count = 3;
    weaponType.stagger = 0.5f;
    Volley volley;
    volley.launch.weaponType = &weaponType;
    volley.launched = 1;
    CHECK(!launchDue(volley, 0.4f) && launchDue(volley, 0.5f) && volleyGoesOn(volley, 0.5f));
}

void testAVolleyEndsOnceAllHaveLeft()
{
    WeaponRec weaponType;
    weaponType.count = 2;
    Volley volley;
    volley.launch.weaponType = &weaponType;
    volley.launched = 2;
    CHECK(!launchDue(volley, 10.f) && !volleyGoesOn(volley, 10.f));
}
}  // namespace

int main()
{
    testAShotLeavesBackAlongItsBoneAtTheCraftsSpeedShare();
    testTheCraftFirstAlongThePathIsHit();
    testACraftWideOfThePathIsNotHit();
    testTheShooterIsNeverNearItsOwnShot();
    testAShotRunningIntoAWallHitsIt();
    testASideHuggingShotSlidesOffTheWall();
    testAShotHuggingBelowSitsItsDistanceOffTheFloor();
    testABounceReflectsTheShotAndLiftsItOffTheSurface();
    testHomingFindsNoTargetOutOfRange();
    testHomingFindsACraftInRangeSooner();
    testTheBeamReaches1000mWithoutATarget();
    testAVolleysProjectilesLeaveOneStaggerApart();
    testAVolleyEndsOnceAllHaveLeft();
    std::printf("shot_flight_tests: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
