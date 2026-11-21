// Argon: where the race's crafts are drawn: a grid slot's craft (the player's and the drones' between their last two
// 60 Hz steps while the race runs, else on their start), the start a slot takes, and the share of a step the poses
// blend by. Implemented by app\race_craft_poses.hpp; the race, the HUD and the scene read it.
#pragma once
#include <glm/glm.hpp>

namespace argon
{
struct StartSlot;
}

class CraftPoses
{
  public:
    virtual glm::mat4 slotMatrix(int slot) const = 0;
    virtual glm::mat4 driverMatrix() const = 0;   // the player's craft between its last two steps
    virtual const argon::StartSlot &startOfSlot(int slot) const = 0;
    virtual float simAlpha() const = 0;           // between the last two 60 Hz steps

  protected:
    ~CraftPoses() = default;
};
