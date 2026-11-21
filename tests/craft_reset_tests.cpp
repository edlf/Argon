// Argon: tests for what puts a craft back on the track (craft +5, respawn 0x5cb30): a hover ray or hull
// contact on a reset triangle, BACK, 5 s upside down, below y -400, a drone's 2 s progress check. A synthetic world
// and empty stats (no gravity or engine), so the craft stays where it is placed. Run with test.bat / test.sh.
// Also: a replayed craft drops the cockpit's shake (0x5ba10 skipped while a replay plays, 0x52afac).
#include "craft/craftsim.hpp"
#include "check.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>

namespace
{
using argon::test::checks;
using argon::test::failures;

constexpr float kStep = 1.f / 60.f;
const glm::vec3 kPlainSpot(0, 1.5f, 20);      // over plain track, the restart node's ground
const glm::vec3 kResetSpot(100, 1.5f, 0);     // over a reset triangle
const glm::vec3 kWallSpot(200, 20.f, 0);      // a reset wall 1 m to the right, nothing below
const glm::vec3 kOpenSpot(300, 50.f, 0);      // nothing anywhere near

void addSquare(std::vector<glm::vec3> &tris, std::vector<argon::TriangleSurface> &surf, glm::vec3 corner, glm::vec3 side1, glm::vec3 side2, bool resets)
{
    const glm::vec3 far = corner + side1 + side2;
    for (const glm::vec3 &p : {corner, corner + side1, far, corner, far, corner + side2})
    {
        tris.push_back(p);
    }
    surf.push_back({0, resets});
    surf.push_back({0, resets});
}

// every stat 0 (no gravity, ride height, engine, friction) but the top speed the steering divides by
argon::CraftStats topSpeedOnly()
{
    const std::filesystem::path file = std::filesystem::temp_directory_path() / "qr_craft_reset_stats.txt";
    std::ofstream(file) << "absolute_max_speed1\t300\n";
    argon::CraftStats stats;
    stats.load(file.string());
    return stats;
}

// track ground around the AI line (x -20..20, z -20..60), a reset patch and a reset wall
struct TestWorld
{
    std::vector<glm::vec3> tris;
    std::vector<argon::TriangleSurface> surf;
    argon::CollisionGrid grid;
    argon::SimWorld world;
    argon::AiGraph graph;
    argon::CraftStats stats = topSpeedOnly();

    TestWorld()
    {
        addSquare(tris, surf, glm::vec3(-20, 0, -20), glm::vec3(40, 0, 0), glm::vec3(0, 0, 80), false);
        addSquare(tris, surf, glm::vec3(90, 0, -10), glm::vec3(20, 0, 0), glm::vec3(0, 0, 20), true);
        addSquare(tris, surf, glm::vec3(201, 10, -10), glm::vec3(0, 20, 0), glm::vec3(0, 0, 20), true);
        grid.build(tris);
        world.grid = &grid;
        world.surf = &surf;
        for (int n = 0; n < 3; ++n)
        {
            argon::AiGraph::Node node;
            node.pos = glm::vec3(0, 0, 20.f * float(n));
            node.down = glm::vec3(0, -1, 0);
            node.fwd = {(n + 1) % 3};
            node.flags = n == 0 ? argon::AiGraph::First | argon::AiGraph::Restart : 0;
            graph.nodes.push_back(node);
        }
        graph.dist = {0.f, 20.f, 40.f};
        graph.length = 60.f;
    }
};

glm::mat4 poseAt(glm::vec3 at, bool upsideDown)
{
    glm::mat4 m(1.f);
    if (upsideDown)
    {
        m[0] = glm::vec4(-1, 0, 0, 0);
        m[1] = glm::vec4(0, -1, 0, 0);
    }
    m[3] = glm::vec4(at, 1.f);
    return m;
}

// a pad-flown craft (no "stuck" check) held still at `at`
argon::GameCraft craftAt(const TestWorld &t, glm::vec3 at, bool upsideDown = false)
{
    argon::GameCraft c;
    c.init(t.stats, 0, 0, 100.f, poseAt(kPlainSpot, false), t.graph, 0, t.world);
    c.flyByPad();
    c.placeFromReplay(poseAt(at, upsideDown), glm::vec3(0));
    return c;
}

void fly(argon::GameCraft &c, const TestWorld &t, float seconds)
{
    for (float done = 0; done < seconds; done += kStep)
    {
        c.frame(kStep, t.world, t.graph);
    }
}

void plainTrackKeepsTheCraft()
{
    TestWorld t;
    argon::GameCraft c = craftAt(t, kPlainSpot);
    fly(c, t, 1.f);
    CHECK(c.respawns() == 0);
}

void aHoverRayOnAResetTriangleRespawns()
{
    TestWorld t;
    argon::GameCraft c = craftAt(t, kResetSpot);
    fly(c, t, kStep);
    CHECK(c.respawns() == 1);
}

void aHullContactWithAResetWallRespawns()
{
    TestWorld t;
    argon::GameCraft c = craftAt(t, kWallSpot);
    fly(c, t, kStep);
    CHECK(c.respawns() == 1);
}

void theRespawnGoesToTheRestartNode()
{
    TestWorld t;
    argon::GameCraft c = craftAt(t, kResetSpot);
    fly(c, t, kStep);
    CHECK(glm::length(glm::vec2(c.position().x, c.position().z)) < 0.01f);
}

void backPressedRespawnsOnce()
{
    TestWorld t;
    argon::GameCraft c = craftAt(t, kOpenSpot);
    argon::GameCraft::Pad back;
    back.reset = true;
    c.flyByPad(back);
    fly(c, t, 1.f);
    CHECK(c.respawns() == 1);
}

void upsideDownUnderFiveSecondsStays()
{
    TestWorld t;
    argon::GameCraft c = craftAt(t, kOpenSpot, true);
    fly(c, t, 4.9f);
    CHECK(c.respawns() == 0);
}

void upsideDownOverFiveSecondsRespawns()
{
    TestWorld t;
    argon::GameCraft c = craftAt(t, kOpenSpot, true);
    fly(c, t, 5.1f);
    CHECK(c.respawns() == 1);
}

void belowMinus400Respawns()
{
    TestWorld t;
    argon::GameCraft c = craftAt(t, glm::vec3(300, -401.f, 0));
    fly(c, t, kStep);
    CHECK(c.respawns() == 1);
}

void aStillDroneWaitsForTheTwoSecondCheck()
{
    TestWorld t;
    argon::GameCraft c = craftAt(t, kPlainSpot);
    c.flyByAi();
    fly(c, t, 1.9f);
    CHECK(c.respawns() == 0);
}

void aStillDroneRespawnsAtTheTwoSecondCheck()
{
    TestWorld t;
    argon::GameCraft c = craftAt(t, kPlainSpot);
    c.flyByAi();
    fly(c, t, 2.1f);
    CHECK(c.respawns() == 1);
}

void aStillDroneWaitsSevenSecondsForTheNextCheck()
{
    TestWorld t;
    argon::GameCraft c = craftAt(t, kPlainSpot);
    c.flyByAi();
    fly(c, t, 8.9f);
    CHECK(c.respawns() == 1);
}

void aPadFlownCraftHasNoProgressCheck()
{
    TestWorld t;
    argon::GameCraft c = craftAt(t, kPlainSpot);
    fly(c, t, 10.f);
    CHECK(c.respawns() == 0);
}

// a blast's knock shakes the internal camera (0x5b8b0) in the next frame
argon::GameCraft knockedCraft(const TestWorld &t)
{
    argon::GameCraft c = craftAt(t, kOpenSpot);
    c.pushByBlast(glm::vec3(100.f, 0.f, 0.f));
    fly(c, t, kStep);
    return c;
}

void aKnockedCraftShakesTheCockpit()
{
    TestWorld t;
    const argon::GameCraft c = knockedCraft(t);
    CHECK(glm::length(c.cameraShake()) > 0.f);
}

void aReplayedCraftHasNoCockpitShake()
{
    TestWorld t;
    argon::GameCraft c = knockedCraft(t);
    c.placeFromReplay(poseAt(kOpenSpot, false), glm::vec3(0));
    CHECK(glm::length(c.cameraShake()) == 0.f);
}
}  // namespace

int main()
{
    plainTrackKeepsTheCraft();
    aHoverRayOnAResetTriangleRespawns();
    aHullContactWithAResetWallRespawns();
    theRespawnGoesToTheRestartNode();
    backPressedRespawnsOnce();
    upsideDownUnderFiveSecondsStays();
    upsideDownOverFiveSecondsRespawns();
    belowMinus400Respawns();
    aStillDroneWaitsForTheTwoSecondCheck();
    aStillDroneRespawnsAtTheTwoSecondCheck();
    aStillDroneWaitsSevenSecondsForTheNextCheck();
    aPadFlownCraftHasNoProgressCheck();
    aKnockedCraftShakesTheCockpit();
    aReplayedCraftHasNoCockpitShake();
    std::printf("craft reset tests: %d checks, %d failed\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
