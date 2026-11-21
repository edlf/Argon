// Argon: a craft's weapon object (craft weapon, one per craft; docs\weapons\README.md): its shield's value and size, the
// TSHIELD it has up, its knock-out, a hit's lasting effects, its two flashes and, for the AI, its power-up levels and
// trigger clocks, with what a hit, a shield and a frame do to them. Which craft is hit, the sounds, the points, the
// replay entries and the knock-out's explosions are App's (weapon_hits.cpp).
#pragma once
#include "logic/ai_trigger.hpp"
#include "logic/hit_flash.hpp"
#include "logic/race_crafts.hpp"
#include "logic/weapon_book.hpp"
#include "logic/weapon_damage.hpp"
#include "logic/weapon_effects.hpp"
#include "logic/weapon_rules.hpp"

#include <cstdint>
#include <vector>

namespace argon::weapons
{

// the TSHIELD (class 2) a craft has up; its bone "<name>0" is drawn on the ship
struct ActiveShield
{
    const WeaponRec *weaponType = nullptr;
    float since = 0;
    uint16_t sphereReached = 0;   // crafts its sphere already met (+0x16d0)
    int bone = -1;                // its bone "<name>0" in the ship model, found when raised (-1: none)
};

// where a shield's sphere is: its owner, its centre (the shield bone) and its radius now
struct ShieldSphereAt
{
    int owner;
    glm::vec3 centre;
    float radius;
};

class CraftWeapon
{
  public:
    // 0x6a862, the weapon object's reset for a race: no shield up, no levels, no knock-out, no effects or flashes (the
    // shield's value and size are set by equipShield; the knock-out's time and shooter are only read once out)
    void resetForRace();
    // the race setup: a shield of `size` (the craft's shieldSize), full
    void equipShield(float size);
    // 0x6ac90: a replay's shield entry sets the value as recorded
    void replayShield(float value);

    // +0, the shield's value (HUD huddata +0x48), 0..shieldMax
    float energy() const
    {
        return energy_;
    }
    float shieldMax() const
    {
        return shieldMax_;
    }
    // huddata +0x44: the shield bar, the position list's energy bars
    float shieldPercent() const;
    const ActiveShield &shield() const
    {
        return shield_;
    }
    // a shield up and within its record's life
    bool shieldUp(float now) const;
    // 0x6aa00: the resistances the shield adds while up
    std::vector<const original::NwpEffects *> shieldBonuses(float now) const;
    // 0x6aba0 / 0x6ab60: the shield up blocks or absorbs
    ShieldGuard guard(float now) const;

    bool knockedOut() const
    {
        return knockedOut_;
    }
    // the shooter that knocked the craft out (the replay's shield entry carries it)
    int knockedOutBy() const
    {
        return knockedOutBy_;
    }
    // +0x1148: the sim clock since the knock-out (the pad's fade, 0x70ac0)
    float secondsKnockedOut(float now) const
    {
        return now - knockedOutAt_;
    }

    // a hit's lasting effects (+0x8..+0x24), handed to the craft's physics (0x5b1d4)
    const CraftEffects &effects() const
    {
        return effects_;
    }
    // +0x98: a human player's red flash on its position list row (docs\weapons\hud.md "Flashes")
    uint32_t hitFlashColour() const
    {
        return hitFlash_.colour();
    }
    float hitFlashLevel() const
    {
        return hitFlash_.level();
    }
    // +0x94 = huddata +0x70: the HUD's shield fill blinks white while above 0
    float shieldFlashLevel() const
    {
        return shieldFlash_.level();
    }

    // 0x6ba30: the AI's power-up levels; the player's are App's hudWeapon / weaponSpecial
    const PowerLevels &aiLoadout() const
    {
        return aiLoadout_;
    }
    void setAiLoadout(const PowerLevels &levels)
    {
        aiLoadout_ = levels;
    }
    // a pickup the AI took: its colour a level up, within `caps`
    void raiseAiLoadout(int pickupType, const PowerLevels &caps);
    // 0x6e9e0: the trigger's clocks this frame; the colours that fire now
    AiFire pullAiTrigger(const AiWants &wants);
    // 0x6ebfc, after any fire: red and blue wait `delay` again
    void delayAiFire(float delay);

    // class 2 (0x6a390): up from `now` for its record's life, drawn at the ship's `bone`
    void raiseShield(const WeaponRec &weaponType, float now, int bone);
    // the launch of a shield with shield resistance (0x6a3fc / 0x6a4df): recharged by `points`, the effects cleared
    void rechargeFromShield(float points);
    // the shield loses `percent` of its size (a negative one recharges it), at most to full
    void loseShield(float percent);
    // the player's mercy (0x6b6ee): the shield left at `percent` of its size
    void spare(float percent);
    // 0x6b7e0: `points` more, at most to full; none once knocked out
    void gainShield(float points);
    // 0x69970: the crafts the shield's sphere reaches for the first time, in racer order; each is marked (+0x16d0)
    std::vector<int> reachWithSphere(const ShieldSphereAt &sphere, const RaceCrafts &crafts);

    // 0x6b1b8: each running effect takes the longer of its and the hit's; true when the hit stops the engine longer
    // than it was (the engine power down sound)
    bool afflict(const CraftEffects &hit);
    // 0x6d000: the effects count down; true when the engine-off time ran out now (the engine power up sound)
    bool countDownEffects(float dt);
    // 0x6d000: the corrode takes its level in % of the shield per second (x share, the nemesis' half), down to 0 at
    // most (it never knocks out); each corroding frame raises the shield flash by 0.2 (0x6d33b)
    void corrode(float dt, float share);
    // 0x6b19a: a hit raises the shield flash, and a human player's hit flash
    void flashShieldHit();
    void flashPlayerHit();
    void fadeFlashes(float dt);
    // 0x6a8a0: out of the race at `now`, by `shooter`; the shield at 0 (the game keeps the overshoot, the HUD bars
    // expect 0..full)
    void knockOut(int shooter, float now);

  private:
    float energy_ = 0;
    float shieldMax_ = 0;   // the craft's shield size, what it starts the race with
    ActiveShield shield_;
    bool knockedOut_ = false;
    float knockedOutAt_ = 0;
    int knockedOutBy_ = 0;
    CraftEffects effects_;   // counted down and corroding each frame (0x6d000)
    HitFlash hitFlash_;
    HitFlash shieldFlash_;
    PowerLevels aiLoadout_;
    AiTrigger aiTrigger_;
};

}  // namespace argon::weapons
