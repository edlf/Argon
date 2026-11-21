// Argon: the weapons' explosions in flight (docs\weapons\explosions.md "Explosions"; the rules: logic\explosion.hpp).
//   impact      0x65f80: bursts A and B whose flags match the impact kind each start `count` explosions `delay`
//               apart, scattered around the impact
//   explosion   0x7e0f0 (a slot of 64 and one of its model's 16 copies, or nothing), 0x7de10 frees it once spent,
//               0x7de80 draws it and makes it go off once
//   knock-out   0x6a8a0: four type 6 explosions carrying nothing
// What going off does (the flash, the push, an area weapon's hit, the sound) is the caller's: update() says which.
#pragma once
#include "logic/explosion.hpp"
#include "logic/viewer_rand.hpp"
#include "logic/weapon_book.hpp"

#include <glm/glm.hpp>

#include <vector>

namespace argon::weapons
{

// an explosion (0x7e0f0): drawn from `due` for `duration`, going off once at `due`
struct Explosion
{
    glm::vec3 at{0.f};
    const WeaponRec *weaponType = nullptr;   // the weapon whose effects it carries (null: none)
    int shooter = 0;
    float due = 0.f, duration = 0.f, scale = 1.f;
    bool damages = false;          // an area weapon's, without the harmless flag 0x10
    bool carriesEffects = false;   // without the harmless flag 0x10: it pushes the crafts around it
    int type = 0;                  // the explosion model (NwpImpactBursts typeA / typeB through explosionModelOf)
    glm::vec3 turnDegrees{0.f};    // yaw, pitch, roll
    bool wentOff = false;
};

// the impact kinds of 0x65f80 (1 craft, 2 world, 3 expired, 4 bounced)
struct Impact
{
    glm::vec3 at;
    const WeaponRec &weaponType;
    int shooter, kind;
};

// what starting explosions draws from: the effects' clock and the viewer's rand
struct ExplosionClock
{
    float now;
    ViewerRand &rand;
};

class Explosions
{
  public:
    explicit Explosions(const WeaponBook &book);

    // burst A then burst B; returns how many explosions the matching bursts asked for, made or not
    int startImpact(const Impact &impact, const ExplosionClock &clock);
    // 0x6a8c3..0x6a9ee: one at the craft, then three around it 0.2 s apart
    void startKnockOut(const glm::vec3 &at, const ExplosionClock &clock);
    // each step: the spent ones go and free their slots, then those whose time has come go off; returns those
    std::vector<Explosion> update(float now);
    // the models of the explosions started since the last call (to load them)
    std::vector<int> takeStartedModels();
    void clear();
    const std::vector<Explosion> &inFlight() const;

  private:
    // an impact burst's explosions (0x65f80 -> 0x7e0f0)
    struct BurstLaunch
    {
        const WeaponRec *weaponType = nullptr;
        int shooter = 0, type = 0, count = 0;
        uint8_t sizePercent = 0;
        float delay = 0.f, scatter = 0.f;
        bool flatScatter = false, carriesEffects = false, damages = false;
    };
    // where a burst starts and when
    struct BurstStart
    {
        glm::vec3 at;
        const ExplosionClock &clock;
    };
    int startBurst(const BurstLaunch &burst, const BurstStart &start);
    void start(const Explosion &explosion, const ExplosionOrder &order, ViewerRand &rand);

    const WeaponBook &book_;
    std::vector<Explosion> inFlight_;
    ExplosionSlots slots_;
    std::vector<int> startedModels_;
};

}  // namespace argon::weapons
