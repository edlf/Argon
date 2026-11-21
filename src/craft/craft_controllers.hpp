// Argon: who flies a craft: the player's pad or the drone's AI (craft +0x850), see craftsim.cpp.
#pragma once
#include "craft/craftsim.hpp"
#include "logic/blast_push.hpp"

namespace argon
{

// ---- who flies the craft: the parts of the craft that differ between CPlayer (vtable 0x19f3e8, player.cpp) and the
// drone (vtable 0x1a1c10, drone.cpp), with the controller at craft +0x850 (pad or AI, its vtable[2] each frame)
class GameCraft::Controller
{
  public:
    virtual void control(GameCraft &craft, float dt, const AiGraph &graph) const = 0;   // fills the control slots +0x8f4
    virtual void sideFriction(GameCraft &craft, float substep) const = 0;                 // vtable 0x54
    virtual void setEngine(GameCraft &craft) const = 0;                             // engine gain +0x9e0, power +0x9e4
    virtual bool respawnsWhenStuck() const = 0;                                 // neon_drone's 2 s progress check
    virtual float blastSideScale() const = 0;                                   // +0x8f0, set by vtable slot 9
    virtual void turbo(GameCraft &craft, float substep) const = 0;                        // force list piece 12

  protected:
    ~Controller() = default;
};

// the player: the pad (player_pad_input 0x70ac0), the brake and airbrake ramps (0x71fe0 / 0x71bc0), the player's
// side friction 0x5ffd0, full engine power; no AI steering assist, no "stuck" respawn
class GameCraft::PlayerController : public GameCraft::Controller
{
  public:
    static const PlayerController shared;

    void control(GameCraft &craft, float dt, const AiGraph &) const override
    {
        craft.padPilot();
        craft.brakeRamp(dt);
        craft.airbrakeRamp(dt);
        craft.turboInput();
    }
    void sideFriction(GameCraft &craft, float substep) const override
    {
        craft.playerSideFriction(substep);
    }
    void setEngine(GameCraft &craft) const override
    {
        craft.gain_ = craft.sp("engine_gain");
        craft.power_ = craft.sp("engine_power");
    }
    bool respawnsWhenStuck() const override
    {
        return false;
    }
    float blastSideScale() const override
    {
        return blast::kPlayerSideScale;
    }
    void turbo(GameCraft &craft, float substep) const override
    {
        craft.turbo(substep);
    }
};

// the drone: the AI pilot (drone_update), the AI's side friction 0x6f720, engine power x its aivalues % and the
// catch-up (0x6d650, set by the viewer), respawned when it stops making progress
class GameCraft::DroneController : public GameCraft::Controller
{
  public:
    static const DroneController shared;

    void control(GameCraft &craft, [[maybe_unused]] float dt, const AiGraph &graph) const override
    {
        craft.aiPilot(graph);
        craft.dropPlayerBrakes();
        craft.testTurboButton();
    }
    void sideFriction(GameCraft &craft, float) const override
    {
        craft.sideFriction();
    }
    void setEngine(GameCraft &craft) const override
    {
        craft.gain_ = craft.sp("engine_gain") + craft.catchGain_;
        craft.power_ = craft.sp("engine_power") * (craft.aiPower_ * 0.01f * craft.catchPower_);
    }
    bool respawnsWhenStuck() const override
    {
        return true;
    }
    float blastSideScale() const override
    {
        return blast::kDroneSideScale;
    }
    void turbo(GameCraft &craft, float substep) const override
    {
        craft.droneTurbo(substep);
    }
};

} // namespace argon
