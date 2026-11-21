// Argon: projectiles, beams and volleys in flight (craftweapon.cpp in the game, docs\weapons\README.md): what they are and the
// rules that only need the crafts (logic\race_crafts.hpp) and the world (WorldRay below).
//   projectile  0x66d30: speed, drop, homing, the craft test 0x683b0, the world ray 0xd4770, the hug rays 0x67753,
//               bounces 0x675f2 (the frame turned along the path: logic\shot_motion.cpp)
//   beam        0x68620 (class 1): from the ship's bone to the target or 1000 m ahead
//   volley      0x6a390 / 0x69e30: projectile k leaves k x stagger after the fire; a repeating one relaunches
// What a step does besides (warnings, sounds, trails, hits, explosions, replay entries) is App's (weapon_shots.cpp).
#pragma once
#include "logic/race_crafts.hpp"
#include "logic/shot_motion.hpp"
#include "logic/viewer_rand.hpp"
#include "logic/weapon_book.hpp"

#include <glm/glm.hpp>

#include <optional>
#include <vector>

namespace argon::weapons
{

struct ShotLaunch
{
    int shooter = 0, colour = 0;
    const WeaponRec *weaponType = nullptr;
};

struct Shot
{
    int shooter = 0, colour = 0, target = -1, bounced = 0, index = 0, volley = 0;
    glm::vec3 right{1, 0, 0};
    const WeaponRec *weaponType = nullptr;
    float born = 0, speed = 0, fall = 0;
    glm::vec3 pos{0}, fwd{0, 0, 1}, up{0, 1, 0}, vel{0}, trailPos{0};
    glm::vec3 beamEnd{0};   // beams (class 1): drawn from pos to here
    bool firstStep = true;  // beams hit on their first frame only (+0x28)
    int bone = -1;          // its bone "<name><index>" in the shooter's ship model, found at launch (-1: none)
};

// a fired weapon slot (weapon object +0xc0 + slot x 0x1730, 0x6a390 / 0x6a6c0): projectile k leaves at
// firedAt + k x stagger (+0x170); a repeating one (+0x13c) leaves again every count x stagger until repeatTime
struct Volley
{
    ShotLaunch launch;
    int id = 0, launched = 0;
    float firedAt = 0;
    std::vector<int> targets;   // per projectile, -1 none
};

struct Segment
{
    glm::vec3 from, to;
};

// where a ray met the world
struct WorldContact
{
    glm::vec3 point, normal;
};

// the world's ray (0xd4770): the first surface between two points
class WorldRay
{
  public:
    virtual ~WorldRay() = default;
    virtual std::optional<WorldContact> along(const glm::vec3 &from, const glm::vec3 &to) const = 0;
};

struct WorldHit
{
    bool hit = false;
    bool slid = false;   // a side-hugging shot moved off the wall instead of hitting it (0x674e7)
    glm::vec3 point{0.f}, normal{0.f};
};

// how the shooter moves when a projectile leaves it (0x64e20)
struct ShooterMotion
{
    float forwardSpeed;
    glm::vec3 velocity, forward;
};

// where and when projectile k leaves: its bone on the ship
struct ShotStart
{
    glm::mat4 bone;
    int index;
    float now;
};

float shotHitRadius(const WeaponRec &weaponType);
Shot makeShot(const ShotLaunch &launch, const ShotStart &start, const ShooterMotion &shooter);
void setShotFrame(Shot &shot, const ShotFrame &frame);
ShotFrame shotFrameOf(const Shot &shot);
// keep the frame square to the forward (homing and bounces turn it)
void squareShotFrame(Shot &shot);

// 0x65a7d..0x65b6a: a pitched record flies along the craft nose up; a lock-on record with +0x167 looks at its target
void aimAtLaunch(Shot &shot, const glm::mat4 &shooterFrame, std::optional<glm::vec3> target);
// 0x66e45..: the speed grows (not after a bounce with +0x156) and the drop grows
void gatherSpeed(Shot &shot);
// homing (0x66d30): another craft in the race inside the lock range, 1 in 8 chance each (rand per craft in range)
int homingTarget(const Shot &shot, const RaceCrafts &crafts, ViewerRand &rand);
// turned towards the target's position by the record's turn rate
void steerTowards(Shot &shot, const glm::vec3 &target);
void moveShot(Shot &shot);
// 0x67c7c: at the end of the step a record without bounces moves by its inherited velocity, which then decays
void carryInheritedVelocity(Shot &shot);

// 0x683b0: the craft in the race whose closest approach to the path is within the radius, first along it; -1 none
int craftHitAlong(int shooter, const Segment &path, float radius, const RaceCrafts &crafts);
// 0x66e60: every other racer within 300 m of a projectile
std::vector<int> craftsNear(const glm::vec3 &point, int shooter, const RaceCrafts &crafts);
// 0x683b0 (0x68591): every other racer in the path's way (argon::pathWarns)
std::vector<int> craftsOnPath(int shooter, const Segment &path, float radius, const RaceCrafts &crafts);

// the world ray old -> now; a side-hugging record (+0x154) without a craft hit this step is put +0x15c off the wall
// and flies on (0x674e7; the game subtracts its normal, the viewer turns the normal back towards the shot)
WorldHit worldHitAlong(Shot &shot, const glm::vec3 &old, bool craftHit, const WorldRay &world);
// 0x67753: the hug rays below, above (unless already hugged) and to the sides; the last normal met (or the wall slid
// along) is the surface the frame then turns along
std::optional<glm::vec3> hugSurfaces(Shot &shot, const WorldHit &wall, const WorldRay &world);
// 0x675a6: while it has bounces left (+0x16c)
bool canBounce(const Shot &shot);
// 0x675f2: 0.4 m off the surface, the frame along the reflected path, the drop cleared
void bounceOff(Shot &shot, const WorldHit &wall);

// 0x68620: the locked target, else 1000 m along the shooter's forward
glm::vec3 beamAimPoint(const Shot &shot, const glm::mat4 &shooterFrame, std::optional<glm::vec3> target);

// 0x69e30: projectile `launched` is due once launched x stagger has passed (a repeating one while its time lasts)
bool launchDue(const Volley &volley, float now);
// the volley stays while projectiles are still to leave or a repeating one's time lasts
bool volleyGoesOn(const Volley &volley, float now);

}  // namespace argon::weapons
