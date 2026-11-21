// Argon: what moves on a craft with its throttle: the jets (0x50960 / 0x50670: the size +0x7c, 0.1 idle .. 0.5 full
// throttle, 1.0 in a turbo) and the airbrake flaps (craft\craft_flaps.hpp: the openings its class keeps), per slot.
#pragma once
#include "craft/craft_flaps.hpp"
#include "effects/effect_slots.hpp"

#include <array>

namespace argon
{

class GameCraft;

// what sizes a jet: the throttle (0..100) and whether the turbo is on
struct JetDrive
{
    float throttle;
    bool turbo;
};

class CraftExhausts
{
  public:
    static constexpr float kIdleJet = 0.1f;

    CraftExhausts();
    void updateJet(int slot, const JetDrive &drive, float dt);
    void updateFlaps(int slot, const GameCraft &craft, float dt);
    // kIdleJet for a slot out of range
    float jetSize(int slot) const;
    const FlapState &flaps(int slot) const;

  private:
    static bool inRange(int slot);

    std::array<float, kEffectSlots> jets_;
    std::array<FlapState, kEffectSlots> flaps_{};
};

}  // namespace argon
