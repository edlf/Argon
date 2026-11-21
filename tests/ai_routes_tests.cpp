// Argon: tests for the AI's route at the AI nodes' forks (logic/ai_routes.hpp): the list 0xe1370 gives each craft, the
// branch each of the strategy's lists takes (0xe3460 / 0xe3740 / 0xe35d0 / 0xe16e0) and the route table's rows
// (0xe38c0). Run with test.bat / test.sh.
#include "logic/ai_routes.hpp"
#include "check.hpp"

#include <cstdio>

namespace
{
using argon::test::checks;
using argon::test::failures;

using argon::airoutes::BranchFacts;
using argon::airoutes::RouteList;
using argon::airoutes::RouteNode;
using argon::airoutes::RoutePreference;
using argon::airoutes::RouteRacer;
using argon::weapons::DroneKind;

RouteRacer generalRacer(int pilot, int lapCounter, int slot)
{
    RouteRacer r;
    r.pilot = pilot;
    r.lapCounter = lapCounter;
    r.slot = slot;
    return r;
}

BranchFacts onSurface(int surface, float percent)
{
    BranchFacts facts;
    facts.surfacePercent[size_t(surface)] = percent;
    return facts;
}

BranchFacts withPickups(int pickups)
{
    BranchFacts facts;
    facts.pickups = pickups;
    return facts;
}

RouteNode fork(const BranchFacts &first, const BranchFacts &second)
{
    RouteNode node;
    node.next = {10, 20};
    node.branches = {first, second};
    return node;
}

RoutePreference taking(RouteList list)
{
    RoutePreference preference;
    preference.list = list;
    return preference;
}

// 0 -> {1, 2} -> 3 -> 0, branch 1 (node 2) on water
std::vector<RouteNode> waterSplitLoop()
{
    std::vector<RouteNode> nodes(4);
    nodes[0] = fork(onSurface(argon::airoutes::kTrackSurface, 100.f), onSurface(argon::airoutes::kWaterSurface, 100.f));
    nodes[0].next = {1, 2};
    nodes[1].next = {3};
    nodes[2].next = {3};
    nodes[3].next = {0};
    return nodes;
}

void aNemesisRaceTakesTheShortestList()
{
    RouteRacer racer = generalRacer(0, 1, 3);
    racer.nemesisRace = true;
    CHECK(argon::airoutes::chooseRouteList(racer) == RouteList::Shortest);
}

void theNemesisAndAHomePilotTakeTheShortestList()
{
    RouteRacer nemesis = generalRacer(8, 1, 1);
    nemesis.kind = DroneKind::Nemesis;
    RouteRacer home = generalRacer(5, 2, 4);
    home.kind = DroneKind::HomeCircuit;
    CHECK(argon::airoutes::chooseRouteList(nemesis) == RouteList::Shortest);
    CHECK(argon::airoutes::chooseRouteList(home) == RouteList::Shortest);
}

void otherCraftsTurnByPilotLapAndSlot()
{
    CHECK(argon::airoutes::chooseRouteList(generalRacer(0, 1, 7)) == RouteList::Pickups);     // 8 & 7 = 0
    CHECK(argon::airoutes::chooseRouteList(generalRacer(0, 1, 0)) == RouteList::TurboGood);   // 1
    CHECK(argon::airoutes::chooseRouteList(generalRacer(1, 1, 0)) == RouteList::Shortest);    // 2
    CHECK(argon::airoutes::chooseRouteList(generalRacer(1, 1, 1)) == RouteList::Track);       // 3
    CHECK(argon::airoutes::chooseRouteList(generalRacer(1, 2, 1)) == RouteList::Water);       // 4
    CHECK(argon::airoutes::chooseRouteList(generalRacer(2, 2, 1)) == RouteList::Rough);       // 5
    CHECK(argon::airoutes::chooseRouteList(generalRacer(2, 3, 1)) == RouteList::Loose);       // 6
    CHECK(argon::airoutes::chooseRouteList(generalRacer(3, 3, 1)) == RouteList::Slippery);    // 7
}

void aSurfaceListTakesTheBranchWithMostOfItsSurface()
{
    const RouteNode node = fork(onSurface(argon::airoutes::kRoughSurface, 40.f), onSurface(argon::airoutes::kRoughSurface, 60.f));
    CHECK(argon::airoutes::branchOf(node, taking(RouteList::Rough)) == 1);
}

void aSurfaceTieGoesToTheFirstBranch()
{
    const RouteNode node = fork(onSurface(argon::airoutes::kLooseSurface, 50.f), onSurface(argon::airoutes::kLooseSurface, 50.f));
    CHECK(argon::airoutes::branchOf(node, taking(RouteList::Loose)) == 0);
}

void noShareOfTheSurfaceTakesTheFirstBranch()
{
    const RouteNode node = fork(onSurface(argon::airoutes::kTrackSurface, 100.f), onSurface(argon::airoutes::kTrackSurface, 100.f));
    CHECK(argon::airoutes::branchOf(node, taking(RouteList::Slippery)) == 0);
}

void eachSurfaceListReadsItsOwnSurface()
{
    const RouteNode water = fork(onSurface(argon::airoutes::kTrackSurface, 100.f), onSurface(argon::airoutes::kWaterSurface, 10.f));
    CHECK(argon::airoutes::branchOf(water, taking(RouteList::Water)) == 1);
    CHECK(argon::airoutes::branchOf(water, taking(RouteList::Track)) == 0);
    const RouteNode slippery = fork(BranchFacts{}, onSurface(argon::airoutes::kSlipperySurface, 10.f));
    CHECK(argon::airoutes::branchOf(slippery, taking(RouteList::Slippery)) == 1);
}

void theTurboListReadsTheRaceSpeedClass()
{
    BranchFacts fastTurbo;
    fastTurbo.turboGoodPercent[3] = 80.f;
    RoutePreference fast = taking(RouteList::TurboGood);
    fast.speedClass = 3;
    RoutePreference slow = taking(RouteList::TurboGood);
    slow.speedClass = 0;
    const RouteNode node = fork(BranchFacts{}, fastTurbo);
    CHECK(argon::airoutes::branchOf(node, fast) == 1);
    CHECK(argon::airoutes::branchOf(node, slow) == 0);
}

void thePickupListRollsTheSeedOverThePickups()
{
    const RouteNode node = fork(withPickups(2), withPickups(6));
    RoutePreference low = taking(RouteList::Pickups);
    low.seed = 9;   // 9 % 8 = 1 < 2
    RoutePreference high = taking(RouteList::Pickups);
    high.seed = 13;   // 13 % 8 = 5
    CHECK(argon::airoutes::branchOf(node, low) == 0);
    CHECK(argon::airoutes::branchOf(node, high) == 1);
}

void withoutPickupsThePickupListTakesTheSeedModTheLinks()
{
    const RouteNode node = fork(BranchFacts{}, BranchFacts{});
    RoutePreference odd = taking(RouteList::Pickups);
    odd.seed = 7;
    CHECK(argon::airoutes::branchOf(node, odd) == 1);
}

void theShortestListTakesTheStoredBranch()
{
    RouteNode node = fork(BranchFacts{}, BranchFacts{});
    node.shortest = 1;
    CHECK(argon::airoutes::branchOf(node, taking(RouteList::Shortest)) == 1);
}

void aSingleLinkIsAlwaysTaken()
{
    RouteNode node;
    node.next = {5};
    CHECK(argon::airoutes::branchOf(node, taking(RouteList::Pickups)) == 0);
}

void twoBranchesStoreTheFirstAsShortest()
{
    CHECK(argon::airoutes::shortestBranch({500.f, 100.f}) == 0);
}

void threeBranchesStoreTheShortest()
{
    CHECK(argon::airoutes::shortestBranch({100.f, 200.f, 300.f}) == 0);
    CHECK(argon::airoutes::shortestBranch({300.f, 100.f, 200.f}) == 1);
    CHECK(argon::airoutes::shortestBranch({300.f, 200.f, 100.f}) == 2);
}

void aThreeWayTieGoesToTheLaterBranch()
{
    CHECK(argon::airoutes::shortestBranch({100.f, 100.f, 300.f}) == 1);
}

void theLookAheadFollowsTheListAtTheFork()
{
    const std::vector<RouteNode> nodes = waterSplitLoop();
    const auto water = argon::airoutes::lookAhead(nodes, 0, taking(RouteList::Water));
    const auto track = argon::airoutes::lookAhead(nodes, 0, taking(RouteList::Track));
    CHECK(water[0] == 0 && water[1] == 2 && water[2] == 3 && water[3] == 0 && water[4] == 2);
    CHECK(track[1] == 1 && track[2] == 3);
}

void aDeadEndRepeatsItsNode()
{
    std::vector<RouteNode> nodes(2);
    nodes[0].next = {1};
    const auto list = argon::airoutes::lookAhead(nodes, 0, taking(RouteList::Track));
    CHECK(list[1] == 1 && list[15] == 1);
}

void aBranchRowGivesEachSurfaceItsShareOfTheNodes()
{
    std::vector<argon::airoutes::ArmNode> arm(4);
    arm[3].surface = argon::airoutes::kWaterSurface;
    const BranchFacts facts = argon::airoutes::branchFacts(arm);
    CHECK(facts.surfacePercent[argon::airoutes::kTrackSurface] == 75.f);
    CHECK(facts.surfacePercent[argon::airoutes::kWaterSurface] == 25.f);
}

void aNodeWithoutGroundCountsInNoSurface()
{
    std::vector<argon::airoutes::ArmNode> arm(2);
    arm[1].surface = argon::airoutes::kNoSurface;
    const BranchFacts facts = argon::airoutes::branchFacts(arm);
    CHECK(facts.surfacePercent[argon::airoutes::kTrackSurface] == 50.f);
}

void aBranchRowSharesTurboAndAddsPickups()
{
    std::vector<argon::airoutes::ArmNode> arm(2);
    arm[0].turboGoodClasses = 0x5;   // slow and normal
    arm[0].pickups = 2;
    arm[1].turboGoodClasses = 0x1;
    arm[1].pickups = 4;
    const BranchFacts facts = argon::airoutes::branchFacts(arm);
    CHECK(facts.turboGoodPercent[0] == 100.f && facts.turboGoodPercent[2] == 50.f && facts.turboGoodPercent[1] == 0.f);
    CHECK(facts.pickups == 6);
}
}  // namespace

int main()
{
    aNemesisRaceTakesTheShortestList();
    theNemesisAndAHomePilotTakeTheShortestList();
    otherCraftsTurnByPilotLapAndSlot();
    aSurfaceListTakesTheBranchWithMostOfItsSurface();
    aSurfaceTieGoesToTheFirstBranch();
    noShareOfTheSurfaceTakesTheFirstBranch();
    eachSurfaceListReadsItsOwnSurface();
    theTurboListReadsTheRaceSpeedClass();
    thePickupListRollsTheSeedOverThePickups();
    withoutPickupsThePickupListTakesTheSeedModTheLinks();
    theShortestListTakesTheStoredBranch();
    aSingleLinkIsAlwaysTaken();
    twoBranchesStoreTheFirstAsShortest();
    threeBranchesStoreTheShortest();
    aThreeWayTieGoesToTheLaterBranch();
    theLookAheadFollowsTheListAtTheFork();
    aDeadEndRepeatsItsNode();
    aBranchRowGivesEachSurfaceItsShareOfTheNodes();
    aNodeWithoutGroundCountsInNoSurface();
    aBranchRowSharesTurboAndAddsPickups();
    std::printf("ai routes tests: %d checks, %d failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
