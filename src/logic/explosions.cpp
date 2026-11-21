// Argon: the weapons' explosions in flight (logic\explosions.hpp; 0x65f80, 0x7e0f0, 0x7de10, 0x7de80, 0x6a8a0).
#include "logic/explosions.hpp"

#include <algorithm>
#include <utility>

namespace argon::weapons
{
namespace
{
const uint32_t kAreaWeapon = 0x400;
const uint8_t kBurstOnCraftOrWorld = 1, kBurstOnExpiry = 2, kBurstOnBounce = 8, kBurstHarmless = 0x10;
const int kImpactExpired = 3, kImpactBounced = 4;

uint8_t burstTrigger(int kind)
{
    if (kind == kImpactExpired)
    {
        return kBurstOnExpiry;
    }
    if (kind == kImpactBounced)
    {
        return kBurstOnBounce;
    }
    return kBurstOnCraftOrWorld;
}

TripleRoll rollThree(ViewerRand &rand)
{
    TripleRoll roll;
    roll.first = rand.next();
    roll.second = rand.next();
    roll.third = rand.next();
    return roll;
}
}  // namespace

Explosions::Explosions(const WeaponBook &book) : book_(book)
{
}

// both bursts scatter flat by burst A's type (0x6601f / 0x66310)
int Explosions::startImpact(const Impact &impact, const ExplosionClock &clock)
{
    const original::NwpImpactBursts &bursts = impact.weaponType.impact;
    const bool area = (impact.weaponType.effects.flags & kAreaWeapon) != 0;
    const bool flat = book_.explosionType(bursts.typeA).look.flatScatter;
    const uint8_t flags[2] = {bursts.flagsA, bursts.flagsB}, types[2] = {bursts.typeA, bursts.typeB}, counts[2] = {bursts.countA, bursts.countB};
    const uint8_t sizes[2] = {bursts.sizePercentA, bursts.sizePercentB};
    const float delays[2] = {bursts.delayA, bursts.delayB}, scatters[2] = {impact.weaponType.scatterA, impact.weaponType.scatterB};
    int asked = 0;
    for (int burst = 0; burst < 2; ++burst)
    {
        if (!(flags[burst] & burstTrigger(impact.kind)))
        {
            continue;
        }
        const bool carries = !(flags[burst] & kBurstHarmless);
        const BurstLaunch launch{&impact.weaponType, impact.shooter, types[burst], counts[burst], sizes[burst], delays[burst], scatters[burst],
                                 flat,        carries,        area && carries};
        asked += startBurst(launch, BurstStart{impact.at, clock});
    }
    return asked;
}

// explosion k at the impact + its scatter, k x delay later; the count asked for, made or not
int Explosions::startBurst(const BurstLaunch &burst, const BurstStart &start)
{
    for (int k = 0; k < burst.count; ++k)
    {
        Explosion explosion;
        explosion.at = start.at + burstScatter(rollThree(start.clock.rand), burst.scatter, burst.flatScatter);
        explosion.weaponType = burst.carriesEffects ? burst.weaponType : nullptr;
        explosion.shooter = burst.shooter;
        explosion.due = start.clock.now + float(k) * burst.delay;
        explosion.damages = burst.damages;
        explosion.carriesEffects = burst.carriesEffects;
        explosion.type = burst.type;
        this->start(explosion, burstExplosionOrder(burst.sizePercent), start.clock.rand);
    }
    return burst.count;
}

// 0x7e0f0: its model's copy and a slot, its duration and scale by its type, its random turn
void Explosions::start(const Explosion &explosion, const ExplosionOrder &order, ViewerRand &rand)
{
    const int model = explosionModelOf(explosion.type);
    if (!slots_.take(model))
    {
        return;
    }
    const ExplosionLook &look = book_.explosionType(model).look;
    const ExplosionSize size = explosionSize(look, order);
    Explosion made = explosion;
    made.type = model;
    made.duration = size.duration;
    made.scale = size.scale;
    made.turnDegrees = explosionTurnDegrees(look, rollThree(rand));
    inFlight_.push_back(made);
    startedModels_.push_back(model);
}

void Explosions::startKnockOut(const glm::vec3 &at, const ExplosionClock &clock)
{
    Explosion blast;
    blast.at = at;
    blast.due = clock.now;
    blast.type = kKnockOutExplosionType;
    start(blast, knockOutBlastOrder(), clock.rand);
    for (int k = 0; k < kKnockOutEchoes; ++k)
    {
        Explosion echo = blast;
        echo.at = at + knockOutEchoOffset(rollThree(clock.rand));
        echo.due = clock.now + knockOutEchoDelay(k);
        start(echo, knockOutEchoOrder(), clock.rand);
    }
}

std::vector<Explosion> Explosions::update(float now)
{
    std::vector<Explosion> goingOff;
    for (size_t i = 0; i < inFlight_.size();)
    {
        Explosion &explosion = inFlight_[i];
        if (explosionSpent(explosion.due, explosion.duration, now))
        {
            slots_.release(explosion.type);
            inFlight_[i] = inFlight_.back();
            inFlight_.pop_back();
            continue;
        }
        if (!explosion.wentOff && explosionShows(explosion.due, explosion.duration, now))
        {
            explosion.wentOff = true;
            goingOff.push_back(explosion);
        }
        ++i;
    }
    return goingOff;
}

std::vector<int> Explosions::takeStartedModels()
{
    return std::exchange(startedModels_, {});
}

void Explosions::clear()
{
    inFlight_.clear();
    slots_.clear();
    startedModels_.clear();
}

const std::vector<Explosion> &Explosions::inFlight() const
{
    return inFlight_;
}

}  // namespace argon::weapons
