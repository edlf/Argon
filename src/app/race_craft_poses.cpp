#include "app/race_craft_poses.hpp"

#include <algorithm>

#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "app/race_session.hpp"
#include "craft/craftsim.hpp"
#include "logic/grid_starts.hpp"
#include "logic/race.hpp"
#include "logic/tick_accumulator.hpp"
#include "render/gpu_model.hpp"

// the grid slot's pose: the driven craft, a drone while the race runs, else its start matrix
glm::mat4 RaceCraftPoses::slotMatrix(int slot) const
{
    const RaceSession &session = sources_.session;
    const bool racing = session.simStarted && (session.driving || sources_.player.lapTime() > 0.f);
    if (slot == 0 && racing)
    {
        return driverMatrix();
    }
    if (racing)
    {
        for (const Race::Drone &drone : sources_.race.grid())
        {
            if (drone.slot == slot)
            {
                return sources_.race.droneMatrix(drone);
            }
        }
    }
    return glm::make_mat4(startOfSlot(slot).matrix);
}

// the craft's pose blended between the last two physics steps
glm::mat4 RaceCraftPoses::driverMatrix() const
{
    const float blend = simAlpha();
    const glm::mat4 &previous = sources_.playerPrev, &current = sources_.playerCur;
    const glm::quat q0 = glm::quat_cast(glm::mat3(previous)), q1 = glm::quat_cast(glm::mat3(current));
    glm::mat4 pose = glm::mat4_cast(glm::slerp(q0, q1, blend));
    pose[3] = glm::mix(previous[3], current[3], blend);
    return pose;
}

// craft `slot` of the race's crafts on player_start<crafts - 1 - slot> (0x3de29)
const argon::StartSlot &RaceCraftPoses::startOfSlot(int slot) const
{
    const auto &starts = sources_.track.data.starts;
    const size_t index = argon::grid::startIndexOf(starts, argon::grid::startNumberOfCraft(slot, sources_.race.plannedRacers()));
    return starts[std::min(index, starts.size() - 1)];
}

float RaceCraftPoses::simAlpha() const
{
    return sources_.ticks.tickShare();
}
