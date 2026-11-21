// Argon: the race's craft poses (logic\craft_poses.hpp) from the grid's starts, the race's drones, the player's last
// two 60 Hz poses and the frames' tick share.
#pragma once
#include "logic/craft_poses.hpp"

namespace argon
{
class GameCraft;
class TickAccumulator;
}  // namespace argon
class Race;
struct RaceSession;
struct Track;

struct RaceCraftPoseSources
{
    const Track &track;                       // the grid's starts
    const Race &race;                         // the drones, the racers planned
    const argon::GameCraft &player;           // its lap time: the race has run
    const glm::mat4 &playerPrev, &playerCur;  // the player's craft at the last two 60 Hz steps
    const argon::TickAccumulator &ticks;      // the share of the next step
    const RaceSession &session;               // the crafts placed and stepped
};

class RaceCraftPoses : public CraftPoses
{
  public:
    explicit RaceCraftPoses(const RaceCraftPoseSources &sources) : sources_(sources)
    {
    }

    glm::mat4 slotMatrix(int slot) const override;
    glm::mat4 driverMatrix() const override;
    const argon::StartSlot &startOfSlot(int slot) const override;
    float simAlpha() const override;

  private:
    RaceCraftPoseSources sources_;
};
