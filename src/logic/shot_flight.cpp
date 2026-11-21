// Argon: projectiles, beams and volleys in flight (logic\shot_flight.hpp; 0x64e20, 0x65a7d, 0x66d30, 0x66e60, 0x67753,
// 0x675f2, 0x67c7c, 0x683b0, 0x68620, 0x69e30).
#include "logic/shot_flight.hpp"
#include "logic/attack_warnings.hpp"

#include <algorithm>

namespace argon::weapons
{
namespace
{
const float kShotStep = 1.f / 60.f;
const float kDefaultHitRadius = 4.f;
const float kBeamReach = 1000.f;   // 0x1a34d8
const float kBounceLift = 0.4f;    // 0x1a3550

// a hug ray: the shot put the hug distance off what it meets along `axis`
std::optional<glm::vec3> hugAlong(Shot &shot, const glm::vec3 &axis, const WorldRay &world)
{
    const std::optional<WorldContact> contact = world.along(shot.pos, shot.pos + axis * shot.weaponType->hugDist);
    if (!contact)
    {
        return std::nullopt;
    }
    shot.pos = contact->point - axis * shot.weaponType->hugDist;
    return contact->normal;
}
}  // namespace

float shotHitRadius(const WeaponRec &weaponType)
{
    return weaponType.radiusFlag ? weaponType.hitRadius : kDefaultHitRadius;
}

// 0x64d20 / 0x64e20: the start bone's frame and position (back along its forward), speed = forward speed x +0x198 % +
// +0x19c, the inherited velocity
Shot makeShot(const ShotLaunch &launch, const ShotStart &start, const ShooterMotion &shooter)
{
    const WeaponRec &weaponType = *launch.weaponType;
    Shot shot;
    shot.shooter = launch.shooter;
    shot.colour = launch.colour;
    shot.weaponType = &weaponType;
    shot.born = start.now;
    shot.index = start.index;
    shot.fwd = glm::normalize(glm::vec3(start.bone[2]));
    shot.up = glm::normalize(glm::vec3(start.bone[1]));
    shot.right = glm::normalize(glm::vec3(start.bone[0]));
    shot.pos = glm::vec3(start.bone[3]) - shot.fwd * weaponType.back;
    shot.speed = shooter.forwardSpeed * weaponType.speedPct * 0.01f + weaponType.speedAdd;
    shot.vel = inheritedVelocity(shooter.velocity, shooter.forwardSpeed, shooter.forward, weaponType.inherit);
    shot.trailPos = shot.pos;
    shot.beamEnd = shot.pos;
    return shot;
}

void setShotFrame(Shot &shot, const ShotFrame &frame)
{
    shot.right = frame.right;
    shot.up = frame.up;
    shot.fwd = frame.forward;
}

ShotFrame shotFrameOf(const Shot &shot)
{
    return ShotFrame{shot.right, shot.up, shot.fwd};
}

void squareShotFrame(Shot &shot)
{
    const glm::vec3 side = shot.right;
    shot.up = shot.up - shot.fwd * glm::dot(shot.up, shot.fwd);
    if (glm::length(shot.up) > 1e-4f)
    {
        shot.up = glm::normalize(shot.up);
    }
    const glm::vec3 right = glm::cross(shot.up, shot.fwd);
    if (glm::length(right) > 1e-4f)
    {
        shot.right = glm::normalize(right) * (glm::dot(right, side) < 0.f ? -1.f : 1.f);
    }
}

void aimAtLaunch(Shot &shot, const glm::mat4 &shooterFrame, std::optional<glm::vec3> target)
{
    const WeaponRec &weaponType = *shot.weaponType;
    if (weaponType.cls != 0)
    {
        return;
    }
    if (weaponType.pitchedLaunch)
    {
        const ShotFrame axes{glm::normalize(glm::vec3(shooterFrame[0])), glm::normalize(glm::vec3(shooterFrame[1])),
                             glm::normalize(glm::vec3(shooterFrame[2]))};
        setShotFrame(shot, frameRaisedFrom(axes, weaponType.launchPitch));
        return;
    }
    if (weaponType.lockOn && weaponType.aimsAtLock && target)
    {
        setShotFrame(shot, frameAimedAt(shot.pos + shot.fwd * weaponType.back, *target, shotFrameOf(shot)));
    }
}

void gatherSpeed(Shot &shot)
{
    const WeaponRec &weaponType = *shot.weaponType;
    shot.speed = acceleratedSpeed(shot.speed, weaponType.accel, weaponType.maxSpeed, kShotStep, weaponType.bounces && shot.bounced > 0 && weaponType.holdsSpeedAfterBounce);
    shot.fall += kShotStep * weaponType.drop;
}

int homingTarget(const Shot &shot, const RaceCrafts &crafts, ViewerRand &rand)
{
    for (int racer = 0; racer < crafts.racerCount(); ++racer)
    {
        const int slot = crafts.slotOf(racer);
        if (slot == shot.shooter || !crafts.targetable(slot))
        {
            continue;
        }
        if (crafts.flies(slot) && glm::length(crafts.position(slot) - shot.pos) < shot.weaponType->lockRange && (rand.next() & 7) == 0)
        {
            return slot;
        }
    }
    return -1;
}

void steerTowards(Shot &shot, const glm::vec3 &target)
{
    const glm::vec3 to = target - shot.pos;
    if (glm::length(to) <= 1e-3f)
    {
        return;
    }
    const float turnShare = std::min(kShotStep * shot.weaponType->turn * 60.f, 100.f) * 0.01f;
    shot.fwd = glm::normalize(shot.fwd + (glm::normalize(to) - shot.fwd) * turnShare);
}

void moveShot(Shot &shot)
{
    shot.pos += shot.fwd * shot.speed * kShotStep;
    shot.pos.y -= shot.fall * kShotStep;
}

void carryInheritedVelocity(Shot &shot)
{
    if (shot.weaponType->bounces)
    {
        return;
    }
    shot.pos += shot.vel * kShotStep;
    shot.vel = decayedInherited(shot.vel, kShotStep);
}

int craftHitAlong(int shooter, const Segment &path, float radius, const RaceCrafts &crafts)
{
    if (glm::distance(path.from, path.to) <= 1e-4f)
    {
        return -1;
    }
    float best = 1e30f;
    int hit = -1;
    for (int racer = 0; racer < crafts.racerCount(); ++racer)
    {
        const int slot = crafts.slotOf(racer);
        if (slot == shooter || !crafts.targetable(slot))
        {
            continue;
        }
        const PathOffset offset = pathOffset(path.from, path.to, crafts.position(slot));
        if (offset.distance > radius || offset.along <= -radius || offset.along >= offset.length + radius || offset.along >= best)
        {
            continue;
        }
        best = offset.along;
        hit = slot;
    }
    return hit;
}

std::vector<int> craftsNear(const glm::vec3 &point, int shooter, const RaceCrafts &crafts)
{
    std::vector<int> near;
    for (int racer = 0; racer < crafts.racerCount(); ++racer)
    {
        const int slot = crafts.slotOf(racer);
        if (slot != shooter && glm::distance(crafts.position(slot), point) < kProjectileWarningRange)
        {
            near.push_back(slot);
        }
    }
    return near;
}

std::vector<int> craftsOnPath(int shooter, const Segment &path, float radius, const RaceCrafts &crafts)
{
    std::vector<int> onPath;
    for (int racer = 0; racer < crafts.racerCount(); ++racer)
    {
        const int slot = crafts.slotOf(racer);
        if (slot != shooter && pathWarns(pathOffset(path.from, path.to, crafts.position(slot)), radius))
        {
            onPath.push_back(slot);
        }
    }
    return onPath;
}

WorldHit worldHitAlong(Shot &shot, const glm::vec3 &old, bool craftHit, const WorldRay &world)
{
    WorldHit hit;
    const std::optional<WorldContact> contact = world.along(old, shot.pos);
    if (!contact)
    {
        return hit;
    }
    hit.hit = true;
    hit.point = contact->point;
    hit.normal = contact->normal;
    if (craftHit || !shot.weaponType->hugSide)
    {
        return hit;
    }
    const glm::vec3 back = glm::dot(hit.normal, shot.pos - old) > 0.f ? -hit.normal : hit.normal;
    shot.pos = hit.point + glm::normalize(back) * shot.weaponType->hugDist;
    hit.hit = false;
    hit.slid = true;
    return hit;
}

std::optional<glm::vec3> hugSurfaces(Shot &shot, const WorldHit &wall, const WorldRay &world)
{
    std::optional<glm::vec3> hugged;
    if (wall.slid)
    {
        hugged = wall.normal;
    }
    if (shot.weaponType->hugBelow)
    {
        if (const std::optional<glm::vec3> normal = hugAlong(shot, -shot.up, world))
        {
            shot.fall = 0.f;
            hugged = normal;
        }
    }
    if (shot.weaponType->hugAbove && !hugged)
    {
        if (const std::optional<glm::vec3> normal = hugAlong(shot, shot.up, world))
        {
            shot.fall = 0.f;
            hugged = normal;
        }
    }
    if (shot.weaponType->hugSide)
    {
        std::optional<glm::vec3> normal = hugAlong(shot, -shot.right, world);
        if (!normal)
        {
            normal = hugAlong(shot, shot.right, world);
        }
        if (normal)
        {
            hugged = normal;
        }
    }
    return hugged;
}

bool canBounce(const Shot &shot)
{
    return shot.weaponType->bounces && shot.bounced < shot.weaponType->maxBounces;
}

void bounceOff(Shot &shot, const WorldHit &wall)
{
    ++shot.bounced;
    glm::vec3 normal = glm::normalize(wall.normal);
    if (glm::dot(normal, shot.fwd) > 0.f)
    {
        normal = -normal;
    }
    shot.pos = wall.point + normal * kBounceLift;
    shot.fall = 0.f;
    setShotFrame(shot, frameAlong(shot.fwd - 2.f * glm::dot(shot.fwd, normal) * normal, shotFrameOf(shot)));
}

glm::vec3 beamAimPoint(const Shot &shot, const glm::mat4 &shooterFrame, std::optional<glm::vec3> target)
{
    if (target)
    {
        return *target;
    }
    return shot.pos + glm::normalize(glm::vec3(shooterFrame[2])) * kBeamReach;
}

bool launchDue(const Volley &volley, float now)
{
    const WeaponRec &weaponType = *volley.launch.weaponType;
    const float age = now - volley.firedAt;
    if (volley.launched >= weaponType.count && !(weaponType.repeats && weaponType.stagger > 0.f && age < weaponType.repeatTime))
    {
        return false;
    }
    return age >= float(volley.launched) * weaponType.stagger;
}

bool volleyGoesOn(const Volley &volley, float now)
{
    const WeaponRec &weaponType = *volley.launch.weaponType;
    return volley.launched < weaponType.count || (weaponType.repeats && now - volley.firedAt < weaponType.repeatTime);
}

}  // namespace argon::weapons
