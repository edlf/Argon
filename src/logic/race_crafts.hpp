// Argon: what the weapons see of the race: the racers (the game's craft list) by grid slot, where each is, which way it
// faces and whether it can still be aimed at. The race (App over logic\race.hpp) implements it; the weapons' rules take
// it instead of App.
#pragma once
#include <glm/glm.hpp>

namespace argon::weapons
{

class RaceCrafts
{
  public:
    virtual ~RaceCrafts() = default;
    virtual int racerCount() const = 0;
    virtual int slotOf(int racer) const = 0;
    // a craft is in that slot
    virtual bool flies(int slot) const = 0;
    virtual glm::vec3 position(int slot) const = 0;
    // its matrix as drawn (the player's interpolated frame, a drone's current one): right, up, forward, position
    virtual glm::mat4 frame(int slot) const = 0;
    // still racing: not knocked out
    virtual bool targetable(int slot) const = 0;
};

}  // namespace argon::weapons
