// Argon: the race, the game's race manager (0x596db0: the finish order +0x514, per craft its race slot
// 0x5972f0 + slot x 0x88) and its drones (vtable 0x1a1c10): every grid slot after the player's runs the game's AI pilot
// and craft physics (craftsim.cpp) with the catch-up (0x6d650, config\ai_values_<track>.txt), the overtaking offsets
// (0xe4be0) and the craft contacts (0xcf920 / 0xd4410 / 0xd5140); docs\physics\opponents.md "Opponents". The player's craft is
// App::sim (it also drives the editor); racer 0 is that craft, racers 1.. the drones (logic\race.cpp).
#pragma once
#include "logic/craft_poses.hpp"
#include "util/found.hpp"
#include <array>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "craft/craftsim.hpp"
#include "logic/ai_routes.hpp"
#include "logic/catch_up_rules.hpp"
#include "logic/route_fork_log.hpp"
#include "logic/viewer_rand.hpp"
#include "track/trackdata.hpp"

class DriveCamera;
class GameModes;
class MultiplayerSession;
class StartGrid;
class Weapons;
struct Track;

// what the race reads and changes
struct RaceSources
{
    const std::string &gameFolder;   // config\ai_values_<track>.txt
    const Track &track;
    GameModes &modes;                 // the mode's drones, lap limit
    const MultiplayerSession &multiplayer;
    const argon::AiGraph &aiGraph;
    argon::SimWorld &simWorld;
    const argon::CraftStats &craftStats;
    StartGrid &startGrid;
    argon::GameCraft &player;         // racer 0 (App::sim)
    const DriveCamera &driveView;     // a human's chase camera
    const Weapons &weapons;           // the drones' kinds
    argon::ViewerRand &rand;
    const int &speedClass, &hudLaps;
    const float &aiPower;
    const bool &playerDrive;
    const CraftPoses &poses;          // the starts, the drones' blend between steps
};

// what the race asks App for
class RaceHost
{
  public:
    virtual argon::GameCraft::Pad readPlayerPad(int player) = 0;
    virtual void raiseDust(const argon::GameCraft &craft) = 0;
    virtual void followCraftEffects(const argon::GameCraft &craft, const glm::mat4 &matrix, int slot) = 0;
    virtual void aiTakePickups(const glm::vec3 &ship, int slot) = 0;
    virtual void playerLapFinished(float lapTime) = 0;
    virtual float lapTurbo(int character) const = 0;
    virtual bool nemesisRace() const = 0;
    virtual bool raceRunning() const = 0;   // not at the race end (the turbo then stops)

  protected:
    ~RaceHost() = default;
};

class Race
{
  public:
    static constexpr int kMaxRacers = 16;

    struct Drone
    {
        int slot = 0;                    // its grid slot (the trails, sprays and weapons index by slot)
        argon::GameCraft craft;
        glm::mat4 prev{1}, cur{1};       // its pose at the last two 60 Hz steps (drawn interpolated)
        float overtakeTimer = 0;
        int overtakeSide = 0;
        int player = -1;   // split screen: the human player (1..3) flying it on their pad, else -1 (the AI)
        argon::GameCraft::Tripod camPrev{}, camCur{};   // a human's chase camera at the last two steps
        int turboLaps = 0;                           // a human's laps already granted their turbo
    };

    Race(RaceHost &host, const RaceSources &sources) : host(host), sources(sources)
    {
    }
    Race(const Race &) = delete;
    Race &operator=(const Race &) = delete;

    // the grid
    void loadCatchUp();        // config\ai_values_<track>.txt, with the track
    void fillGrid();           // a drone on each start slot after the player's (the game mode's count, 0x79250)
    int plannedRacers() const;   // the crafts fillGrid puts on the grid, the player included
    void holdOnGrid();         // a countdown step: the drones settle on the pad, their timers wait for GO
    void startDrones();        // GO (0x78270): the AI flies them, the lap timers start (0x63a30)
    void seatPlayers(int players);   // split screen: players 2..n fly the first drones (grid slots 1..n-1)
    void step();               // one 60 Hz step after the player's craft moved: catch-up, overtaking, the drones, contacts
    void placeFromReplay(int racer, const glm::mat4 &pose, const glm::vec3 &velocity);   // a drone placed, not flown

    // the racers
    int racerCount() const
    {
        return 1 + int(drones.size());
    }
    const argon::GameCraft &racer(int i) const;
    int slotOf(int racer) const
    {
        return racer == 0 ? 0 : drones[size_t(racer - 1)].slot;
    }
    Found<const argon::GameCraft> craftInSlot(int slot) const;   // the player's or a drone's; none when the slot is empty
    Found<argon::GameCraft> craftInSlot(int slot);
    bool slotRacing(int slot) const;
    const std::vector<Drone> &grid() const
    {
        return drones;
    }
    glm::mat4 droneMatrix(const Drone &d) const;        // between its last two steps, as App::driverMatrix
    float distance(const argon::GameCraft &c) const;       // laps x 100 + % of the lap, as the game keeps it
    std::vector<int> places() const;                    // 1-based, by race distance

    // the standings (the race manager): each racer's finished laps, race time, kills and the finish order
    void resetStandings();
    // View Replay puts the crafts back on the grid: the race's results are kept aside and put back after it, the lap
    // counters taken as they are then (the replayed laps are not counted again), the knocked-out drones grounded again
    struct Standings
    {
        std::vector<int> finished, knockedOutOrder;
        std::array<float, kMaxRacers> totals{};
        std::array<int, kMaxRacers> kills{};
    };
    Standings standings() const;
    void restoreStandings(const Standings &kept);
    void stepStandings();      // each 60 Hz step: a finished lap adds its time; the last one puts the racer in the order
    void countKill(int shooter);
    // a craft knocked out by weapons (0x58d20): out of the race, placed from the back of the finish order (0x638b0);
    // a drone stops flying
    void knockOut(int slot);
    void finishByPlace();      // 0x3f320 (the player's finish, the time limit): the racers still racing join by place
    bool knockedOut(int racer) const;
    const std::vector<int> &finishOrder() const
    {
        return finished;
    }
    float raceTime(int racer) const
    {
        return totals[racer];
    }
    int kills(int racer) const
    {
        return killCount[racer];
    }
    // the finish order, the others by their place now, the knocked out last (logic\results_rules.hpp)
    std::vector<int> resultOrder() const;
    bool playerWon() const
    {
        return !finished.empty() && finished[0] == 0;
    }
    bool playerFinished() const;

    // the viewer's switches and counters (the side panel, the tests)
    int droneCount = 5;        // quick race: 6 crafts (the game's 12 trails = 6 ships x 2)
    bool catchUpOn = true, contactsOn = true;
    bool routesOn = true;      // off: every craft takes each fork's first link (the viewer before the route choice)
    const std::vector<RouteForkLog::Entry> &forksTaken() const   // each fork a racer passed (logic\route_fork_log.hpp)
    {
        return forkLog.entries();
    }
    int contacts() const
    {
        return contactCount;
    }
    bool catchUpLoaded() const
    {
        return catchUpTable.loaded;
    }

  private:
    struct CatchUpTable
    {      // ai_values_<track>.txt, rows per speed class unless noted
        using SpeedRow = argon::catchup::SpeedRow;
        bool loaded = false;
        SpeedRow headDist{}, headMod{};
        std::array<SpeedRow, 5> zoneMin{}, zoneMax{};          // [zone]
        std::array<argon::catchup::ZoneGainRows, 5> zoneGain{};   // [zone]
        SpeedRow zonePower{};                                  // per zone
        std::array<SpeedRow, 16> pilotGain{};                  // [AI pilot]
        std::array<argon::catchup::PlayerIsRows, 16> playerIs{};  // [the player's pilot]
    };
    void readCatchUpRow(const std::string &key, const CatchUpTable::SpeedRow &row);
    void readZoneRow(const std::string &key, const CatchUpTable::SpeedRow &row);
    void readPlayerIsRow(const std::string &key, const CatchUpTable::SpeedRow &row);
    void catchUp(argon::GameCraft &c, int racer, const std::vector<int> &place) const;
    void overtake(Drone &drone, int racer, const std::vector<int> &place);
    void stepDrone(Drone &drone);
    void stepHuman(Drone &drone);
    void grantLapTurbo(Drone &drone);
    void collidePairs();
    std::vector<int> racingByPlace() const;
    void chooseRoutes();        // 0xe1370 for each AI-flown racer (the player's craft too when the AI flies it)
    argon::airoutes::RouteRacer routeRacerOf(int racerIndex) const;

    RaceHost &host;
    RaceSources sources;
    std::vector<Drone> drones;
    CatchUpTable catchUpTable;
    int contactCount = 0;
    std::vector<int> finished;
    std::vector<int> knockedOutOrder;   // racers, first knocked out first
    float totals[kMaxRacers]{};
    int lapsCounted[kMaxRacers]{}, killCount[kMaxRacers]{};
    RouteForkLog forkLog;
};
