// Argon: the start-up's test runs: --turbo-test, the AI drive's pre-run (--drive / --drive-time), the ghost replay
// (--ghost-replay) and the drive and race reports on stdout.
#include "launch/test_runs.hpp"

#include <algorithm>
#include <cstdio>

#include "app/weapons.hpp"
#include "audio/audio.hpp"
#include "craft/drive.hpp"
#include "effects/wake_spray.hpp"
#include "effects/weapon_sprites.hpp"
#include "logic/particles.hpp"
#include "logic/pickups.hpp"
#include "logic/race.hpp"
#include "logic/race_scoring.hpp"
#include "logic/replay_race.hpp"
#include "logic/start_grid.hpp"

namespace
{
const char *routeListName(std::optional<argon::airoutes::RouteList> list)
{
    static const char *const kNames[] = {"track", "water", "loose", "slippery", "rough", "shortest", "turbo good", "pickups"};
    return list ? kNames[int(*list)] : "first links";
}

const char *droneKindName(argon::weapons::DroneKind kind)
{
    if (kind == argon::weapons::DroneKind::Nemesis)
    {
        return "nemesis";
    }
    return kind == argon::weapons::DroneKind::HomeCircuit ? "home" : "general";
}

} // namespace

void TestRuns::applyTurboTest()
{
    const float seconds = options_.drive.turboTestSeconds;
    if (seconds < 0)
    {
        return;
    }
    sources_.player.addTurbo(seconds);
    sources_.player.setTurboTest(true);
}

// ------------------------------------------------------------------ --drive / --drive-time: the AI drive, pre-run
void TestRuns::preRunDrive()
{
    const DriveStart &driveStart = options_.drive;
    if (!driveStart.enabled || !sources_.route.valid())
    {
        return;
    }
    sources_.driving = true;
    sources_.chaseCam = true;
    applyTurboTest();
    const int frames = int(driveStart.preRunSeconds * 60.f + 0.5f);
    const int logEvery = driveStart.logEverySeconds > 0 ? std::max(1, int(driveStart.logEverySeconds * 60.f + 0.5f)) : 0;
    for (int frame = 0; frame < frames; ++frame)
    {
        host_.stepDrive(1.f / 60.f);
        if (logEvery && frame % logEvery == 0)
        {
            printDriveState(frame);
        }
    }
    if (driveStart.preRunSeconds > 0)
    {
        printDriveReport();
        std::fflush(stdout);   // the whole report reaches a redirected output while the viewer runs on
    }
    preRunGhostReplay();
    host_.settleDrive(1.f);   // settle the chase camera
}

// --ghost-replay g s: the pause menu's Replay Loaded (0) / Session Ghost (1), stepped s seconds
void TestRuns::preRunGhostReplay()
{
    const RaceTestStart &raceTest = options_.raceTest;
    if (raceTest.ghostReplay < 0)
    {
        return;
    }
    host_.startGhostReplay(raceTest.ghostReplay);
    std::printf("ghost replay %d: %s\n", raceTest.ghostReplay, sources_.replayRace.playing() ? "playing" : "no such ghost");
    const int frames = int(raceTest.ghostReplaySeconds * 60.f + 0.5f);
    for (int frame = 0; frame < frames; ++frame)
    {
        host_.stepDrive(1.f / 60.f);
    }
}

void TestRuns::printDriveState(int frame) const
{
    const argon::GameCraft &sim = sources_.player;
    const glm::vec3 position = sim.position();
    std::printf("  turbo %4.1f%s", sim.turboSeconds(), sim.turboOn() ? "*" : " ");
    std::printf("  t %6.1f  %4.0f km/h  gear %d revs %4.0f thr %3.0f  %s%s node %3d  lap %d  err %+.3f  hits %d resp %d  "
                "pos %.0f %.1f %.0f  hov %d [%.2f %.2f %.2f %.2f]  eng %.0f  rumble %u %u  wake %d spray %zu drops %zu sprites %zu",
                frame / 60.f,
                sim.speedKmh(),
                sim.gear() + 1,
                sim.revs(),
                sim.throttle(),
                sim.onGround() ? sim.surfaceName() : "air",
                sim.inWater() ? "/water" : "",
                sim.node(),
                sim.laps(),
                sim.steerError(),
                sim.contacts(),
                sim.respawns(),
                position.x,
                position.y,
                position.z,
                sim.hoverContacts(),
                sim.hoverDepth(0),
                sim.hoverDepth(1),
                sim.hoverDepth(2),
                sim.hoverDepth(3),
                sources_.audio.throttle,
                unsigned(sim.rumbleFirst()),
                unsigned(sim.rumbleSecond()),
                sim.wakeCorners(),
                sources_.wakeSpray.puffs(),
                sources_.wakeSpray.drops(),
                sources_.weaponSprites.sprites());
    const glm::vec3 shake = sim.cameraShake();
    const glm::vec3 jolt = sim.contactJolt();
    std::printf("  knock %.2f shake %+.4f %+.4f %+.4f  jolt r/u/f %+.1f %+.1f %+.1f\n", sim.knock(), shake.x, shake.y, shake.z, jolt.x, jolt.y, jolt.z);
}

void TestRuns::printDriveReport() const
{
    const argon::GameCraft &sim = sources_.player;
    std::printf("drive: %.1f s, %s speed %d, %.0f km/h, laps %d, last %.2f s, best %.2f s, wall hits %d, respawns %d\n",
                options_.drive.preRunSeconds,
                argon::pilotName(argon::pilotIndex(sources_.startGrid.pilotOf(0))),
                sources_.speedClass + 1,
                sim.speedKmh(),
                sim.laps(),
                sim.lastLap(),
                sim.bestLap(),
                sim.contacts(),
                sim.respawns());
    std::printf("pickups: %d taken this lap, levels red %d blue %d yellow %d, special %d, %zu particles, score %d "
                "(shown %d), %zu popups\n",
                sources_.pickups.takenThisLapCount(),
                sources_.powerLevels[0],
                sources_.powerLevels[1],
                sources_.powerLevels[2],
                sources_.weaponSpecial,
                sources_.particles.size(),
                sources_.scoring.points.target(),
                sources_.scoring.points.shown(),
                sources_.scoring.points.queuedPopups());
    if (!sources_.race.grid().empty())
    {
        printRaceReport();
    }
}

// the route table's rows at each fork: % track / rough / loose / water / slippery, % turbo_good at the race's speed
// class, pickups; and the fork's shortest branch
void TestRuns::printRouteTable() const
{
    const std::vector<argon::airoutes::RouteNode> &routes = sources_.aiGraph.routes;
    const size_t speed = size_t(std::clamp(sources_.speedClass, 0, argon::airoutes::kSpeedClasses - 1));
    for (size_t n = 0; n < routes.size(); ++n)
    {
        for (size_t k = 0; k < routes[n].branches.size(); ++k)
        {
            const argon::airoutes::BranchFacts &facts = routes[n].branches[k];
            std::printf("route table: node %3zu branch %zu -> %3d  surfaces %3.0f %3.0f %3.0f %3.0f %3.0f  turbo %3.0f  pickups %d%s\n",
                        n,
                        k,
                        routes[n].next[k],
                        facts.surfacePercent[0],
                        facts.surfacePercent[1],
                        facts.surfacePercent[2],
                        facts.surfacePercent[3],
                        facts.surfacePercent[4],
                        facts.turboGoodPercent[speed],
                        facts.pickups,
                        int(k) == routes[n].shortest ? "  shortest" : "");
        }
    }
}

// each fork a racer passed: lap, fork node, the branch its route took (and its list), the branch it was nearest
void TestRuns::printForksTaken() const
{
    for (const RouteForkLog::Entry &entry : sources_.race.forksTaken())
    {
        std::printf(
            "fork: racer %d lap %d node %3d -> branch %d (%s), nearest branch %d\n", entry.racer, entry.lap, entry.node, entry.branch, routeListName(entry.list), entry.nearestBranch);
    }
}

void TestRuns::printRaceReport() const
{
    const Race &race = sources_.race;
    std::printf("race: %d crafts, catch-up %s, ship contacts %d\n", race.racerCount(), race.catchUpLoaded() ? "loaded" : "none", race.contacts());
    std::printf("weapons: %zu shots in flight, %zu volleys, %zu explosions, energy",
                sources_.weapons.shots().size(),
                sources_.weapons.volleys().size(),
                sources_.weapons.explosions().inFlight().size());
    for (int i = 0; i < race.racerCount(); ++i)
    {
        std::printf(" %.0f", sources_.weapons.craft(race.slotOf(i)).energy());
    }
    std::printf(", knocked out");
    for (int i = 0; i < race.racerCount(); ++i)
    {
        if (race.knockedOut(i))
        {
            std::printf(" %d", i);
        }
    }
    std::printf(", kills");
    for (int i = 0; i < race.racerCount(); ++i)
    {
        std::printf(" %d", race.kills(i));
    }
    const Weapons::PlayerLocks &locks = sources_.weapons.locks(0);
    std::printf("\nlocks: red %zu blue %zu yellow %zu, crosshairs", locks.lists()[0].size(), locks.lists()[1].size(), locks.lists()[2].size());
    for (auto &colour : locks.crosshairs())
    {
        for (auto &crosshair : colour)
        {
            if (crosshair.state)
            {
                std::printf(" [%d->%d]", crosshair.state, crosshair.target);
            }
        }
    }
    std::printf("\n");
    printRouteTable();
    printForksTaken();
    const auto place = race.places();
    for (int i = 0; i < race.racerCount(); ++i)
    {
        const auto &racer = race.racer(i);
        const Weapons::Loadout loadout = sources_.weapons.loadoutOf(race.slotOf(i));
        std::printf("  slot %d %-11s %-7s catch-up %+5.2f x%.3f  place %d  distance %6.1f  %4.0f km/h  respawns %d  levels r%d b%d y%d s%d\n",
                    i,
                    argon::pilotName(racer.pilot()),
                    droneKindName(sources_.weapons.droneKindOf(race.slotOf(i))),
                    racer.catchUpGain(),
                    racer.catchUpPower(),
                    place[size_t(i)],
                    race.distance(racer),
                    racer.speedKmh(),
                    racer.respawns(),
                    loadout.red,
                    loadout.blue,
                    loadout.yellow,
                    loadout.special);
    }
}
