// Argon: the race (logic\race.hpp): the drones on the grid with the game's AI, catch-up, overtaking and
// craft contacts, and the race manager's standings. docs\physics\README.md ("Opponents"), docs\frontend\race-end.md "Race end".
#include "logic/race.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <utility>

#include "app/weapons.hpp"
#include "files/game_path.hpp"
#include "logic/drive_camera.hpp"
#include "logic/game_logic.hpp"
#include "logic/multiplayer_session.hpp"
#include "logic/start_grid.hpp"
#include "render/gpu_model.hpp"

#include <glm/gtc/type_ptr.hpp>
#include "logic/results_rules.hpp"

namespace
{
std::vector<std::string> fields(const std::string &line)
{
    std::vector<std::string> out;
    std::stringstream ss(line);
    std::string field;
    while (std::getline(ss, field, '\t'))
    {
        size_t first = field.find_first_not_of(" \r"), last = field.find_last_not_of(" \r");
        out.push_back(first == std::string::npos ? std::string() : field.substr(first, last - first + 1));
    }
    return out;
}

float number(const std::vector<std::string> &fields, int index)
{
    return index < int(fields.size()) && !fields[index].empty() ? float(std::atof(fields[index].c_str())) : 0.f;
}

// a row's five values after its name (the speed classes; ZoneEnginePowerModifier: the zones)
argon::catchup::SpeedRow speedRow(const std::vector<std::string> &v)
{
    argon::catchup::SpeedRow row{};
    for (size_t i = 0; i < row.size(); ++i)
    {
        row[i] = number(v, int(i) + 1);
    }
    return row;
}

// the AI node nearest the start slot: where the drone's racing line starts
int nearestNode(const argon::AiGraph &graph, const glm::vec3 &pos)
{
    int best = 0;
    float bestDistance = 1e30f;
    for (size_t i = 0; i < graph.nodes.size(); ++i)
    {
        float distance = glm::length(graph.nodes[i].pos - pos);
        if (distance < bestDistance)
        {
            bestDistance = distance;
            best = int(i);
        }
    }
    return best;
}
}  // namespace

// ai_values_<track>.txt: rows "name<TAB>v1..v5" (speed classes; ZoneEnginePowerModifier: zones). The first block of
// pilot rows after "Gain Modifiers" is the per-pilot gain table.
void Race::loadCatchUp()
{
    catchUpTable = CatchUpTable{};
    std::ifstream f(argon::findGamePath(sources.gameFolder + "/config/ai_values_" + sources.track.name + ".txt"));
    if (!f)
    {
        return;
    }
    std::string line;
    bool inGain = false;
    while (std::getline(f, line))
    {
        const auto v = fields(line);
        if (v.empty() || v[0].empty())
        {
            continue;
        }
        const std::string &key = v[0];
        if (key.rfind("Gain Modifiers", 0) == 0)
        {
            inGain = true;
            continue;
        }
        if (key.rfind("AI Head Start", 0) == 0)
        {
            inGain = false;
        }
        const CatchUpTable::SpeedRow row = speedRow(v);
        if (inGain)
        {
            for (size_t p = 0; p < catchUpTable.pilotGain.size(); ++p)
            {
                if (key == argon::pilotName(int(p)))
                {
                    catchUpTable.pilotGain[p] = row;
                }
            }
            continue;
        }
        readCatchUpRow(key, row);
    }
    catchUpTable.loaded = true;
}

void Race::readCatchUpRow(const std::string &key, const CatchUpTable::SpeedRow &row)
{
    if (key == "HeadStartDistance")
    {
        catchUpTable.headDist = row;
    }
    else if (key == "HeadStartModifier")
    {
        catchUpTable.headMod = row;
    }
    else if (key == "ZoneEnginePowerModifier")
    {
        catchUpTable.zonePower = row;
    }
    readZoneRow(key, row);
    readPlayerIsRow(key, row);
}

// Zone<n>Min / Max and General_ / HomeCircuit_ / Nemesis_Zone<n>Modifier (0xe5990)
void Race::readZoneRow(const std::string &key, const CatchUpTable::SpeedRow &row)
{
    for (size_t z = 0; z < catchUpTable.zoneGain.size(); ++z)
    {
        const std::string name = std::to_string(z + 1);
        argon::catchup::ZoneGainRows &gain = catchUpTable.zoneGain[z];
        if (key == "Zone" + name + "Min")
        {
            catchUpTable.zoneMin[z] = row;
        }
        else if (key == "Zone" + name + "Max")
        {
            catchUpTable.zoneMax[z] = row;
        }
        else if (key == "General_Zone" + name + "Modifier")
        {
            gain.general = row;
        }
        else if (key == "HomeCircuit_Zone" + name + "Modifier")
        {
            gain.homeCircuit = row;
        }
        else if (key == "Nemesis_Zone" + name + "Modifier")
        {
            gain.nemesis = row;
        }
    }
}

// "Player Is": <pilot>_General_AI_modifier / <pilot>_Nemesis_AI_modifier
void Race::readPlayerIsRow(const std::string &key, const CatchUpTable::SpeedRow &row)
{
    for (size_t p = 0; p < catchUpTable.playerIs.size(); ++p)
    {
        const std::string pilot = argon::pilotName(int(p));
        if (key == pilot + "_General_AI_modifier")
        {
            catchUpTable.playerIs[p].general = row;
        }
        else if (key == pilot + "_Nemesis_AI_modifier")
        {
            catchUpTable.playerIs[p].nemesis = row;
        }
    }
}

// ---- the grid

// the game only builds the crafts that race (0x79250: the player, then the drones; a nemesis race only the nemesis),
// so the start slots past them stay empty
// the player and the game mode's drones (0x79250), at most one a start locator
int Race::plannedRacers() const
{
    const int seats = sources.modes.current().hasOpponents() ? sources.modes.current().droneSeats(droneCount)
                                                             : sources.multiplayer.players() - 1;   // multiplayer: the other players
    return 1 + std::max(0, std::min({seats, int(sources.track.data.starts.size()) - 1, kMaxRacers - 1}));
}

void Race::fillGrid()
{
    drones.clear();
    contactCount = 0;
    forkLog.reset(sources.aiGraph);
    if (!sources.craftStats.loaded() || sources.aiGraph.nodes.size() < 3)
    {
        return;
    }
    const int dronesToPlace = plannedRacers() - 1;
    for (int slot = 1; slot <= dronesToPlace; ++slot)
    {
        const auto &start = sources.poses.startOfSlot(slot);
        Drone drone;
        drone.slot = slot;
        drone.craft.init(sources.craftStats,
                     argon::pilotIndex(sources.startGrid.pilotOf(slot)),
                     sources.speedClass,
                     sources.aiPower,
                     glm::make_mat4(start.matrix),
                     sources.aiGraph,
                     nearestNode(sources.aiGraph, glm::vec3(start.pos[0], start.pos[1], start.pos[2])),
                     sources.simWorld);
        drone.prev = drone.cur = drone.craft.matrix();
        drone.overtakeSide = int(sources.rand.next() & 1);
        drone.craft.setRouteSeed(int(sources.rand.next()));   // the strategy's rand() (0xe4630, +0x54c)
        drones.push_back(std::move(drone));
    }
}

void Race::holdOnGrid()
{
    for (Drone &drone : drones)
    {
        drone.craft.flyByPad();
        drone.prev = drone.cur;
        drone.craft.frame(1.f / 60.f, sources.simWorld, sources.aiGraph);
        drone.craft.resetLapTimer();
        drone.cur = drone.craft.matrix();
        drone.camPrev = drone.camCur;
        drone.camCur = drone.craft.tripod(sources.driveView.tripodIndex());
    }
}

void Race::startDrones()
{
    for (Drone &drone : drones)
    {
        if (drone.player < 0)
        {
            drone.craft.flyByAi();
        }
        drone.craft.resetLapTimer();
    }
}

void Race::seatPlayers(int players)
{
    for (size_t i = 0; i < drones.size(); ++i)
    {
        Drone &drone = drones[i];
        drone.player = int(i) + 1 < players ? int(i) + 1 : -1;
        drone.camPrev = drone.camCur = drone.craft.tripod(sources.driveView.tripodIndex());
    }
}

void Race::placeFromReplay(int racer, const glm::mat4 &pose, const glm::vec3 &velocity)
{
    Drone &drone = drones[size_t(racer - 1)];
    drone.prev = drone.cur;
    drone.craft.placeFromReplay(pose, velocity);
    drone.cur = drone.craft.matrix();
}

const argon::GameCraft &Race::racer(int index) const
{
    return index == 0 ? sources.player : drones[size_t(index - 1)].craft;
}

Found<const argon::GameCraft> Race::craftInSlot(int slot) const
{
    if (slot == 0)
    {
        return sources.player;
    }
    for (const Drone &drone : drones)
    {
        if (drone.slot == slot)
        {
            return drone.craft;
        }
    }
    return {};
}

Found<argon::GameCraft> Race::craftInSlot(int slot)
{
    const Found<const argon::GameCraft> found = std::as_const(*this).craftInSlot(slot);
    return found ? Found<argon::GameCraft>(const_cast<argon::GameCraft &>(*found)) : Found<argon::GameCraft>();
}

bool Race::slotRacing(int slot) const
{
    return static_cast<bool>(craftInSlot(slot));
}

glm::mat4 Race::droneMatrix(const Drone &drone) const
{
    const float blend = sources.poses.simAlpha();
    const glm::quat q0 = glm::quat_cast(glm::mat3(drone.prev)), q1 = glm::quat_cast(glm::mat3(drone.cur));
    glm::mat4 pose = glm::mat4_cast(glm::slerp(q0, q1, blend));
    pose[3] = glm::mix(drone.prev[3], drone.cur[3], blend);
    return pose;
}

float Race::distance(const argon::GameCraft &craft) const
{
    return float(craft.laps()) * 100.f + craft.lapProgress(sources.aiGraph) * 100.f;
}

std::vector<int> Race::places() const
{
    const size_t racers = size_t(racerCount());
    std::vector<int> order(racers), place(racers);
    std::vector<float> d(racers);
    for (size_t i = 0; i < racers; ++i)
    {
        order[i] = int(i);
        d[i] = distance(racer(int(i)));
    }
    std::stable_sort(order.begin(), order.end(), [&](int left, int right) { return d[size_t(left)] > d[size_t(right)]; });
    for (size_t k = 0; k < racers; ++k)
    {
        place[size_t(order[k])] = int(k) + 1;
    }
    return place;
}

// ---- the drones' step

void Race::step()
{
    chooseRoutes();
    for (int i = 0; i < std::min(racerCount(), int(RouteForkLog::kRacers)); ++i)
    {
        forkLog.step(i, racer(i));
    }
    if (drones.empty())
    {
        return;
    }
    const std::vector<int> place = places();
    for (size_t i = 0; i < drones.size(); ++i)
    {
        Drone &drone = drones[i];
        if (drone.player >= 0)
        {
            stepHuman(drone);
            continue;
        }
        catchUp(drone.craft, int(i) + 1, place);
        overtake(drone, int(i) + 1, place);
        stepDrone(drone);
    }
    if (contactsOn)
    {
        collidePairs();
    }
}

// 0x6d650, per AI craft before its frame: gain = engine_gain (+ head start + zone gains) x place factor + pilot gain +
// "player is" gain; power = base x (1 + ZoneEnginePowerModifier / 100) for each zone the gap falls in. The zone gains
// and the "player is" row follow the craft's kind (0x58e40: the nemesis, a pilot on its home track, the others).
void Race::catchUp(argon::GameCraft &c, int racerIndex, const std::vector<int> &place) const
{
    if (!catchUpOn || !catchUpTable.loaded)
    {
        c.setCatchUp(0.f, 1.f);
        return;
    }
    const CatchUpTable &t = catchUpTable;
    const int speed = std::clamp(sources.speedClass, 0, 4);
    const argon::weapons::DroneKind kind = sources.weapons.droneKindOf(slotOf(racerIndex));
    const float craftDistance = distance(c), rp = distance(sources.player);
    float gain = 0.f, power = 1.f;
    if (craftDistance < t.headDist[speed] + 100.f)
    {
        float headMod = t.headMod[speed];
        if (racerCount() == 2)
        {
            static const float twoCraft[5] = {0.f, 2.5f, 3.f, 3.5f, 3.5f};
            headMod = speed == 0 ? 0.f : headMod - twoCraft[speed];
        }
        gain += headMod;
    }
    const float gap = craftDistance - rp;
    for (int z = 0; z < 5; ++z)
    {
        if (gap < t.zoneMax[z][speed] && t.zoneMin[z][speed] <= gap)
        {
            gain += argon::catchup::zoneGain(t.zoneGain[z], kind, speed);
            power *= t.zonePower[z] * 0.01f + 1.f;
        }
    }
    // place among the AI (the player's place skipped), after the first 10 % of the race
    float scale = 1.f;
    if (!(c.laps() < 1 && c.lapProgress(sources.aiGraph) * 100.f <= 10.f))
    {
        int racerPlace = place[size_t(racerIndex)];
        if (place[0] < racerPlace)
        {
            --racerPlace;
        }
        static const float fast[6] = {1, 1, 0.98f, 0.96f, 0.94f, 0.92f}, mid[6] = {1, 1, 0.97f, 0.94f, 0.91f, 0.88f},
                           slow[6] = {1, 0.95f, 0.9f, 0.85f, 0.8f, 0.75f};
        const float *table = speed >= 3 ? fast : speed == 2 ? mid : slow;
        if (racerPlace >= 1 && racerPlace <= 5)
        {
            scale = table[racerPlace];
        }
    }
    char key[32];
    std::snprintf(key, sizeof key, "engine_gain%d", speed + 1);
    const float engineGain = sources.craftStats.get(key, c.pilot());
    const float playerIs = argon::catchup::playerIsGain(t.playerIs[size_t(argon::pilotIndex(sources.startGrid.pilotOf(0)))], kind, speed);
    const float total = (engineGain + gain) * scale + t.pilotGain[size_t(c.pilot())][speed] + playerIs;
    c.setCatchUp(total - engineGain, power);
}

// the "ai dpa" 0xe1370, each frame: the strategy's list for this craft (logic\ai_routes.cpp)
void Race::chooseRoutes()
{
    for (int i = 0; i < racerCount(); ++i)
    {
        argon::GameCraft &craft = *craftInSlot(slotOf(i));
        if (!routesOn || craft.flownByPad())
        {
            craft.followFirstLinks();
            continue;
        }
        craft.followRoute(argon::airoutes::chooseRouteList(routeRacerOf(i)));
    }
}

argon::airoutes::RouteRacer Race::routeRacerOf(int racerIndex) const
{
    const argon::GameCraft &craft = racer(racerIndex);
    argon::airoutes::RouteRacer racer;
    racer.pilot = craft.pilot();
    racer.lapCounter = craft.laps() + 1;   // race slot +0x14 counts the lap being flown
    racer.slot = slotOf(racerIndex);
    racer.nemesisRace = host.nemesisRace();
    racer.kind = sources.weapons.droneKindOf(racer.slot);
    return racer;
}

// overtaking (0xe4be0): the craft one place ahead; the side opposite to its own, re-picked every 8..15 s
void Race::overtake(Drone &drone, int racerIndex, const std::vector<int> &place)
{
    const int me = place[size_t(racerIndex)];
    int ahead = -1;
    for (int j = 0; j < racerCount(); ++j)
    {
        if (place[size_t(j)] == me - 1)
        {
            ahead = j;
        }
    }
    if (me <= 1 || ahead < 0)
    {
        drone.craft.setOvertake(false, 0.f, 0);
        return;
    }
    const float gap = distance(racer(ahead)) - distance(drone.craft);
    if (gap < 5.f)
    {
        drone.overtakeTimer -= 1.f / 60.f;
        if (drone.overtakeTimer < 0.f)
        {
            drone.overtakeTimer = float(sources.rand.next() & 7) + 8.f;
            const int aheadSide = ahead == 0 ? 0 : drones[size_t(ahead - 1)].overtakeSide;
            drone.overtakeSide = aheadSide == 1 ? 0 : 1;
        }
    }
    drone.craft.setOvertake(true, gap, drone.overtakeSide);
}

// the drone's frame, then its dust, trail, jet, spray and the pickups it flies through
void Race::stepDrone(Drone &drone)
{
    drone.prev = drone.cur;
    drone.craft.frame(1.f / 60.f, sources.simWorld, sources.aiGraph);
    drone.cur = drone.craft.matrix();
    host.raiseDust(drone.craft);
    host.followCraftEffects(drone.craft, drone.cur, drone.slot);
    host.aiTakePickups(glm::vec3(drone.cur[3]), drone.slot);
}

// a split-screen player's craft: their pad (after GO; none while held or out), no catch-up or overtaking (the AI's)
void Race::stepHuman(Drone &drone)
{
    const int racer = int(&drone - drones.data()) + 1;
    const bool out = knockedOut(racer);
    const bool done = std::find(finished.begin(), finished.end(), racer) != finished.end();
    if (done && !out)
    {
        drone.craft.flyByAi();   // past the line: the AI flies it on (as the player's)
    }
    else if (sources.playerDrive || out)
    {
        drone.craft.flyByPad(out ? argon::GameCraft::Pad{} : host.readPlayerPad(drone.player));
    }
    else
    {
        drone.craft.flyByAi();   // an AI race (tests): the AI flies every seat
    }
    drone.craft.setCatchUp(0.f, 1.f);
    drone.craft.setOvertake(false, 0.f, 0);
    drone.craft.setTurboAllowed(host.raceRunning());
    grantLapTurbo(drone);
    stepDrone(drone);
    drone.camPrev = drone.camCur;
    drone.camCur = drone.craft.tripod(sources.driveView.tripodIndex());
}

// race_craft_lap 0x3d5a0, which mode 4's update (0x41490) runs for every craft: each finished lap adds 1.5 + 0.5 x
// the character's Turbo upgrade
void Race::grantLapTurbo(Drone &drone)
{
    if (drone.craft.laps() <= drone.turboLaps)
    {
        return;
    }
    drone.turboLaps = drone.craft.laps();
    drone.craft.addTurbo(host.lapTurbo(drone.craft.pilot()));
}

// every pair, once per frame after all crafts moved
void Race::collidePairs()
{
    const int racers = racerCount();
    for (int a = 0; a < racers; ++a)
    {
        for (int b = a + 1; b < racers; ++b)
        {
            argon::GameCraft &ca = a == 0 ? sources.player : drones[size_t(a - 1)].craft;
            if (argon::GameCraft::collidePair(ca, drones[size_t(b - 1)].craft))
            {
                ++contactCount;
            }
        }
    }
}

// ---- the standings

void Race::resetStandings()
{
    finished.clear();
    knockedOutOrder.clear();
    for (int i = 0; i < kMaxRacers; ++i)
    {
        totals[i] = 0.f;
        lapsCounted[i] = 0;
        killCount[i] = 0;
    }
}

// a racer's finished lap adds its time (0x62ba0: slot +0x60, only up to the race's laps); reaching laps + 1 (the
// game's lap counter starts at 1) puts it in the finish order (0x63850). The player's lap: App::playerLapFinished
Race::Standings Race::standings() const
{
    Standings kept{finished, knockedOutOrder, {}, {}};
    std::copy(std::begin(totals), std::end(totals), kept.totals.begin());
    std::copy(std::begin(killCount), std::end(killCount), kept.kills.begin());
    return kept;
}

void Race::restoreStandings(const Standings &kept)
{
    finished = kept.finished;
    knockedOutOrder.clear();
    for (int racer : kept.knockedOutOrder)
    {
        knockOut(slotOf(racer));   // the grid's fresh drones grounded again
    }
    std::copy(kept.totals.begin(), kept.totals.end(), std::begin(totals));
    std::copy(kept.kills.begin(), kept.kills.end(), std::begin(killCount));
    for (int i = 0; i < std::min(racerCount(), int(kMaxRacers)); ++i)
    {
        lapsCounted[i] = racer(i).laps();
    }
}

void Race::stepStandings()
{
    const int racers = std::min(racerCount(), int(kMaxRacers));
    for (int i = 0; i < racers; ++i)
    {
        const argon::GameCraft &craft = racer(i);
        if (craft.laps() <= lapsCounted[i])
        {
            continue;
        }
        if (!sources.modes.current().hasLapLimit() || craft.laps() <= sources.hudLaps)
        {
            totals[i] += craft.lastLap();
        }
        lapsCounted[i] = craft.laps();
        if (i == 0)
        {
            host.playerLapFinished(craft.lastLap());
        }
        const bool alreadyIn = std::find(finished.begin(), finished.end(), i) != finished.end();
        if (sources.modes.current().hasLapLimit() && craft.laps() >= sources.hudLaps && !alreadyIn)
        {
            finished.push_back(i);
        }
    }
}

bool Race::playerFinished() const
{
    return sources.modes.current().hasLapLimit() && sources.player.laps() >= sources.hudLaps;
}

void Race::countKill(int shooter)
{
    if (shooter >= 0 && shooter < kMaxRacers)
    {
        ++killCount[shooter];
    }
}

std::vector<int> Race::resultOrder() const
{
    return argon::results::finishOrder(finished, racingByPlace(), knockedOutOrder);
}

// the racers neither in the finish order nor knocked out, by their place now
std::vector<int> Race::racingByPlace() const
{
    const std::vector<int> place = places();
    std::vector<int> byPlace(place.size());
    for (size_t i = 0; i < place.size(); ++i)
    {
        byPlace[size_t(place[i] - 1)] = int(i);
    }
    std::vector<int> racing;
    for (int racer : byPlace)
    {
        const bool inOrder = std::find(finished.begin(), finished.end(), racer) != finished.end();
        if (!inOrder && !knockedOut(racer))
        {
            racing.push_back(racer);
        }
    }
    return racing;
}

void Race::knockOut(int slot)
{
    for (int racer = 0; racer < racerCount(); ++racer)
    {
        if (slotOf(racer) != slot || knockedOut(racer))
        {
            continue;
        }
        knockedOutOrder.push_back(racer);
        if (racer > 0)
        {
            drones[size_t(racer - 1)].craft.flyByPad();
        }
    }
}

void Race::finishByPlace()
{
    const std::vector<int> racing = racingByPlace();
    finished.insert(finished.end(), racing.begin(), racing.end());
}

bool Race::knockedOut(int racer) const
{
    return std::find(knockedOutOrder.begin(), knockedOutOrder.end(), racer) != knockedOutOrder.end();
}
