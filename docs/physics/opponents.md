# Physics: opponents

## Opponents
Ported in `src\logic\race.cpp` (`Race`: the drones, catch-up, overtaking, contacts, places and the standings; + `craftsim.cpp`
setCatchUp / setOvertake / collidePair).
- Race slots 0x5972f8 + slot x 0x88: +0 % of the lap, +0x14 laps done; race distance = laps x 100 + %. Places 0x596e00
  (1-based); GetCraftAtPos 0x63760.
- Catch-up 0x6d650 (every frame, AI crafts), tables from config\ai_values_<track>.txt (parser 0xe5990, stored at
  0x43a014; index t = track, s = speed class):
  - gain +0x9e0 = engine_gain[s]; if race distance < HeadStartDistance[s] + 100: + HeadStartModifier[s] (2 crafts:
    speed 1 x 0, speeds 2/3/4-5 - 2.5 / 3 / 3.5).
  - dR = AI distance - player distance; for zones 1..5 with ZoneMin[s] <= dR < ZoneMax[s]: gain += Zone modifier[s]
    (Nemesis_Zone<n>Modifier if +0x9f9, 0x43a014 + 0x1ca0; else HomeCircuit_ if +0x9f8, + 0x1660; else General_,
    + 0x1020; each [track 0x52ccf8][speed][zone]), power +0x9e4 = (ZoneEnginePowerModifier[zone] / 100 + 1) x power
    (starting from the base +0x9fc; the same for every kind); zone index -> +0x1804.
  - The flags come from the craft setup 0x58e40 (every craft, every mode; a jump table per pilot id 0x52cdc0[craft]
    at 0x59754): +0x9f8 = the track id 0x52ccf8 is the pilot's id (Tara 0 silver .. Kukailimoku 7 draco, Alicia 8
    alt_silver .. Kutoo 15 alt_draco; e.g. Marcus 5 on bracken), +0x9f9 = slot 0's pilot is this pilot's nemesis (id ^
    8). So the nemesis is a Nemesis craft in any race it is on the grid (quick races and every tournament race seat it
    in slot 1), not only in the nemesis challenge; a nemesis on its own home track still takes the Nemesis rows.
  - after the first 10 % of the race: gain x factor by the AI's place among the AI (the player's place skipped):
    speeds 4-5 0.98 / 0.96 / 0.94 / 0.92 for places 2..5, speed 3 0.97 / 0.94 / 0.91 / 0.88, speeds 1-2
    0.95 / 0.9 / 0.85 / 0.8 / 0.75 for places 1..5.
  - + Gain Modifiers[AI pilot][s] (+ 0x2420) + "Player Is" [player pilot][s]: <player pilot>_Nemesis_AI_modifier
    (+ 0x4c20) for the +0x9f9 craft, <player pilot>_General_AI_modifier (+ 0x3820) for the others (a home circuit
    craft too). No other kind or mode difference in 0x6d650.
  - Ported: `Race::catchUp` takes the kind from `Weapons::droneKindOf` (`argon::weapons::droneKind`) and the rows through
    `argon::catchup::zoneGain` / `playerIsGain` (logic\catch_up_rules.cpp; tests in weapon_rules_tests). The headless
    drive prints each slot's kind and its last catch-up (gain added, power factor). Before 2026-11-16 every drone took
    the General rows. Check: `--menu --char 0 --tourney-race 1 --tourney-done 3 --race-go --ai-race --drive-time 40`:
    Alicia "nemesis catch-up +3.24 x1.050" (General rows: -1.75 x1.050); `--speed 4 --quick-race 5 ... --drive-time
    60`: Marcus "home catch-up +1.83 x1.035" (General: +1.65, zone 2 HomeCircuit 1.5 vs General 1).
  - config\aivalues_<pilot>.txt: 0xe5990 reads only "speed" and BranchPriority1..4 (surface names, 0xe7300) from
    them; the per-track General / Nemesis / Home Circuit gain columns are never read (and are 0 in all 16 files).
- Route choice at the AI nodes' forks (config\ai_<t>.txt nodes with more than one forward link: silver 6 two-way + 1
  three-way, alt_solar 7, alt_silver 4, alt_bracken / kalau 3, alt_mount / solar 2, bracken / alt_draco / alt_flames 1,
  none on draco, flames, fuusoku, alt_fuusoku, alt_kalau, mount, mars, alt_mars).
  - Route table (CAIRouteTable at 0x42edd8, built at the AI file's load, AI::LoadAndInitialise 0xe5610 -> 0xe3230 ->
    0xe3830): one 0x34-byte row per (fork, branch). 0xe38c0 walks the branch from its first node along the first links
    while the node has one backward link (+0x22; up to the merge) and per node casts a segment from node + down x 10 to
    node - down x 10 (0xd4770; "down" is (0, 1, 0) in the files, so up) for the ground's material and 0xce6b0 at the
    lower end for water (water counts when the ground lies below the water's surface; neither: no surface); rows +8 /
    +0xc / +0x10 / +0x14 / +0x18 = % of the nodes on track / rough / loose / water / slippery, +0x20..+0x30 = % with
    turbo_good_<slow .. ultrafast> (node flag 0x2000 << speed class), +0x1c = their pickups (the sum of 0x1b9be4 ..
    0x1b9bf0). Pickups per node: 0xe2520 (the AI file parser) takes each pickup of types 0..3 (blue, red, yellow,
    mega-up; not power chips) to its nearest node and counts it there when its sideways offset from the line to that
    node's first link (normal (-dz, 0, dx)) is within 3 / 6 / 9 / 15 / 20 m (speed class); the count loop runs two
    rounds, so each pickup counts 2. 0xe16e0 (0x1b9bf4 per node, aimap.cpp): the branch whose first node is nearest the
    first node (flag 2) along the first links (0xe1800); with two branches both arms store 0 (as shipped), with three:
    0 if it is strictly the least, else 1 if 1 < 2, else 2.
  - Lists: the strategy (drone +0x20f8; 0xe46a0 -> 0xe4730 every frame) keeps 8 lists of the 16 nodes from the craft's
    node (0x5972f0 + slot x 0x88), each rebuilt when that node changes, each taking at every fork: list 0 0xe47f0,
    1 0xe4850, 2 0xe4970, 3 0xe4910, 4 0xe48b0 = 0xe3460 with surface 2 track / 3 water / 5 loose / 6 slippy / 4 rough
    (the codes of 0xe7300): the first branch with the largest share of that surface above 0, else branch 0; list 5
    0xe4ae0: 0x1b9bf4 (the shortest); list 6 0xe49d0: 0xe3740, the same with the turbo_good share of the race's speed
    class; list 7 0xe4a50: 0xe35d0, r = the strategy's rand() drawn once at its init (0xe4630, +0x54c): with pickups on
    the fork's branches, the first branch whose running pickup sum exceeds r % the total, else |r % links|. A node with
    one link takes it in every list.
  - Choice ("ai dpa" at drone +0x1c90, 0xe1370 every frame from 0x6de2a, drones only): copies one list (published at
    +0x464, list k at +0x10 + 0x20 k) to +8, which the tactics (0xe4be0 through "ai DPADP", 0xe51a0) steer along. A
    nemesis race (0x791f0, 0xe139b) -> list 5. Else race slot +0x19 (= craft +0x9f9, the nemesis flag, copied at
    0x594a0 by the craft setup 0x58e40) -> list 7 if the strategy's +0x549 is set, else list 5; but 0xe4730 rebuilds
    list 5 (0xe4ae0, which clears +0x549) after list 7 (0xe4a50, which sets it), so the nemesis always takes list 5.
    Else slot +0x1a (= craft +0x9f8, home circuit, 0x594b5) -> list 5. Else (pilot id 0x52cdc0[slot] + laps +0x14 +
    slot) & 7: 0 -> 7, 1 -> 6, 2 -> 5, 3 -> 0, 4 -> 1, 5 -> 4, 6 -> 2, 7 -> 3; so the choice changes each lap.
  - config\aivalues_<pilot>.txt BranchPriority1..4 (0xe5990 -> 0xe7300 into 0x43a030 + pilot x 16 + 4..0x10): no
    reader found (no immediate reference to 0x43a034..0x43a130); the lists' surface priorities are constants.
  - Ported: logic\ai_routes.cpp (`argon::airoutes`: chooseRouteList, branchOf, lookAhead, shortestBranch, branchFacts;
    tests in ai_routes_tests), the table from the track in ai_route_table.cpp (`AiGraph::buildRoutes`, at each race
    start with its speed class; the pickups' positions from pup.txt without the per-type lift). `Race::chooseRoutes`
    gives each AI-flown craft its list every step (the player's craft too when the AI flies it, as slot 0; laps =
    finished laps + 1); the craft's look-ahead, its node along the route (trackProgress), its lap % and its restart
    point follow that list (`GameCraft::routeNext`). Side branch nodes' lap % lie evenly between the fork's and the
    merge's (as 0xe4460 fills the nodes the routes leave at 0; the game's other nodes take the ai file's route lengths,
    0xe3c90, not ported). Before 2026-11-16 every craft took each fork's first link (`--first-branch` keeps that).
    Check: `--menu --quick-race 0 --race-go --ai-race --drive-time 240` prints the table ("route table: node n branch
    k -> first node, surfaces % track rough loose water slippery, turbo % at the race's speed class, pickups,
    shortest") and "fork: racer r lap l node n -> branch b (list), nearest branch b" for every fork passed (nearest =
    the branch whose path, fork to merge, lies nearest the craft half a second later, logic\route_fork_log.cpp).
    Silver, speed 3, 240 s: 99 forks passed, branches taken at node 28 0 / 1 / 2 = 7 / 12 / 5, at node 5 24 / 3, at
    node 124 22 / 2 (then node 74 branch 1), every one on the branch it flew; with `--first-branch` 96 forks, all
    branch 0. Bracken (one fork, node 23): Marcus (home) and Alicia (nemesis) shortest every lap, Tara (slot 0) turbo
    good / shortest / track on laps 1 / 2 / 3. Nemesis challenge (`--char 0 --tourney-race 1 --tourney-done 3`): both
    crafts shortest. Not checked live: the ground / water heights 0xe38c0 compares (taken as the hit point's y), the
    rows' values against xemu (silver's arms come out mostly loose; some nodes hit no ground within 10 m).
- Overtaking (tactics 0xe4be0): if not leading and the craft one place ahead is less than 5 % of a lap away, the
  three aim points move by up x (P1 - P3), normalised, x (10 - 3 gap), each component clamped to +-5, to the left
  or right (side per craft at 0x5939c4+0x33ec: opposite to the craft ahead's, re-picked when its timer
  0x5939c4+0x3410 runs out, every 8 + (rand & 7) s).
- Craft vs craft: physics_objects_contacts 0xcf920 (once per frame after every object stepped) puts each craft's hull
  box into a 3-axis sweep and prune (0xd5840, pairs 0xd4d30 = overlapping on all three axes); 0xd4410 makes one
  contact per pair: point between the box centres, normal = normalize(centre b - centre a). contact_response
  0xd5140, two-body branch: J = m_a (v_a - v_b) . n; if J > 0: J += 5, P_a -= 0.7 J n, v_a = P_a / m_a,
  P_b += 0.7 J n, v_b = P_a / m_a (as shipped). No positional push-out.
- Start positions (0x3de29): craft i of the race's n takes the locator "player_start<n - 1 - i>" (locator.dat, found
  by name 0x59f90), so the player starts last and craft n - 1 on player_start0 (the pole); Time Attack's lone craft
  on player_start0. Argon: logic\grid_starts.hpp, RaceCraftPoses::startOfSlot (before 2026-11-16 the player had the pole).
- Grid (race setup 0x79250 in the game state 0x5293b0, slots +0x3a10.. = 0x52cdc0..): slot 0 the player, slot 1 the
  player's nemesis (id < 8 ? id + 8 : id - 8, kept at +0x3a14); a nemesis challenge race (0x791f0) has only those two:
  0x79250 skips modes 2 / 3 / 4, then on 0x791f0 calls 0x78d60 with 2 (the craft count +0x3a0c) and returns, without
  the extra pilot or the random fill; otherwise 0x78d60 with 6 after 0x82960. Everything sized by the craft count
  (the grid flyby, the HUD position list, the results, the 2-craft head start modifier above) then has two entries;
  the nemesis takes the Nemesis catch-up rows (as in every race, above) and route list 5 (0xe139b, "Route choice"
  above). Argon:
  `TournamentLogic::droneSeats` (`argon::tourney::droneSeats`) gives `Race::fillGrid` 1 drone there, on start slot 1
  (before 2026-11-16 it kept the 5 drones of the other races). Test: `--menu --char 0 --tourney-race 1
  --tourney-done 3 --race-go --drive-time 3` prints "race: 2 crafts".
  Otherwise slot 5 = the extra pilot at +0x3948 if it is a valid id other than the two (else it is random too) and
  0x82960 fills slots 2.. at random: id = rand / 0x888, retried until it is in the unlocked mask (profile +0x5b0,
  at least 7 set), not the player and not on the grid. Quick race: 6 crafts. Argon draws the random slots from
  the profile's unlocked characters too (`GridRule::unlockedCharacters`; the nemesis is added locked or not) and
  ignores +0x3948 ("New grid" re-rolls).
- AI crafts take pickups too (0x6d000 with the AI flag): no once-per-lap rule, power chips skipped, burst on them.
- AI hairpin brake limit (control 0xe0810 at 0xe0e21, the "ai control" object at drone +0x180c): on a hairpin
  (+0x47a, node flag 0x80000 via 0xe0f40) throttle and brake are cleared while f > clamp(+0x470 x 0.5, 0.3, 1).
  Nothing in the XBE writes control +0x470 (= drone +0x1c7c): no store to either offset, not in the drone
  constructors (0x3f9c4 / 0x403d4 / 0x40f44 set +0x180c, +0x1810..+0x1818, +0x1c74, +0x1c8c) nor in its init
  0xe0770; aivalues_<pilot>.txt and the modes do not reach it. So the limit is 0.3 whenever the drone's memory
  (heap 0xb3ca0) starts zeroed, which argon assumes (craftsim.cpp aiPilot); not checked live.
- Argon: the HUD position list and minimap icons are drawn (HUD section).
