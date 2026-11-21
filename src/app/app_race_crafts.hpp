// Argon: the race's crafts and the world as the weapons' rules see them (logic\race_crafts.hpp, logic\shot_flight.hpp),
// from the race, the crafts' weapon objects (their frames) and the simulation's world.
#pragma once
#include "logic/race_crafts.hpp"
#include "logic/shot_flight.hpp"

class Race;
class Weapons;

class AppRaceCrafts final : public argon::weapons::RaceCrafts
{
  public:
    AppRaceCrafts(const Race &race, const Weapons &weapons) : race_(race), weapons_(weapons)
    {
    }
    int racerCount() const override;
    int slotOf(int racer) const override;
    bool flies(int slot) const override;
    glm::vec3 position(int slot) const override;
    glm::mat4 frame(int slot) const override;
    bool targetable(int slot) const override;

  private:
    const Race &race_;
    const Weapons &weapons_;
};

namespace argon
{
struct SimWorld;
}

// the simulation's world ray (0xd4770)
class SimWorldRay final : public argon::weapons::WorldRay
{
  public:
    explicit SimWorldRay(const argon::SimWorld &world) : world_(world)
    {
    }
    std::optional<argon::weapons::WorldContact> along(const glm::vec3 &from, const glm::vec3 &to) const override;

  private:
    const argon::SimWorld &world_;
};
