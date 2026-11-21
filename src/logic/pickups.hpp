// Argon: the track's power-up pickups as the game runs them (docs\effects\pickups.md "Pickups"): pickups_collide 0x8fcd0
// (called from the craft weapon update 0x6d000 with the ship's position, reach 14), the respawn 0.4 s after a take,
// the player's once-per-lap rule (the flags 0x52c860, cleared when a lap starts, 0x63153). What a take gives (power-up
// levels 0x6ba30, points, sounds, the bursts) is the caller's; this class says which pickup went or came back.
#pragma once
#include "track/trackdata.hpp"

#include <glm/glm.hpp>

#include <optional>
#include <string>
#include <vector>

namespace argon
{

// the game's pickup types (0x78380), from pup.txt's names
enum class PickupType
{
    Blue = 0,
    Red = 1,
    Yellow = 2,
    MegaUp = 3,
    PowerChip = 4,   // points only, no weapon levels; never comes back
};
PickupType pickupTypeOf(const std::string &name);
// the starburst's colour (ARGB) per type; 0 for the mega-up (each direction a random red / yellow / blue)
uint32_t pickupBurstColour(PickupType type);

// one pickup taken or back on the track: which, its type, where it stands
struct PickupChange
{
    enum class Kind
    {
        Taken,
        Back
    };
    Kind kind;
    int index;
    PickupType type;
    glm::vec3 pos;
};

class Pickups
{
  public:
    // the track's pickups, all on the track and none taken
    void place(const std::vector<Pickup> &spots);
    // the "taken this lap" flags clear when the player's lap count changes
    void startLap(int lap);
    // a 60 Hz step of the player at `ship`: the pickups back (0.4 s after a take) and the ones taken, in index order
    std::vector<PickupChange> stepPlayer(const glm::vec3 &ship, float now);
    // an AI craft at `ship` (0x6d000 with the AI flag): no once-per-lap rule, power chips left alone
    std::vector<PickupChange> takeByAi(const glm::vec3 &ship, float now);
    // a replay's entries (signals 8 / 9); nothing for an index the track has not
    std::optional<PickupChange> takeReplayed(int index, float now);
    std::optional<PickupChange> bringBack(int index);

    size_t count() const;
    bool onTrack(size_t index) const;
    bool takenThisLap(size_t index) const;
    int takenThisLapCount() const;
    const std::string &typeName(size_t index) const;
    // where it floats: the pup.txt spot raised by its type's lift (0x78380)
    glm::vec3 position(size_t index) const;

  private:
    struct Spot
    {
        std::string typeName;
        PickupType type;
        glm::vec3 pos;
        bool onTrack = true, takenThisLap = false;
        float takenAt = -1e9f;
    };
    PickupChange changeOf(PickupChange::Kind kind, size_t index) const;
    PickupChange take(size_t index, float now);
    bool known(int index) const;

    std::vector<Spot> spots_;
    int lap_ = 0;
};

}  // namespace argon
