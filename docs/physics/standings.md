# Physics: standings

## Standings
The race manager 0x596db0 (update 0x62ba0 every frame) keeps a race slot per racer at 0x5972f8 + racer x 0x88
(manager +0x548): +0 % of the lap (+8 the last frame's), +0x10 the lap counter (starts at 1), +0x14 the line crossings
(a backward wrap takes one off), +0x50 the current lap's time, +0x54 the last lap, +0x58 the best lap, +0x5c "timing
started", +0x60 the race time, +0x6a DNF; the finish order at +0x514 (0x5972c4), the places at +0x50 (0x596e00).
- Each frame, per slot: before the first crossing dt goes to +0x60, after it to +0x50. A wrap of the lap % (a drop
  below -90) with the counter equal to the crossings completes a lap: the best lap takes the minimum, +0x60 += the
  lap's time only while the counter is below laps + 1 (or in mode 3), +0x54 = the lap, +0x50 = 0; then the crossings
  +1 and, below laps + 1, the counter +1; reaching laps + 1 puts the racer in the order (0x63850). So the slots never
  stop: the crafts race on after the finish (the AI flies them, laps go on counting crossings and best laps), but a
  finished racer's race time and lap counter stay as they were at its line.
- Places (+0x50): while 0x52afa8 = 0 (no race-end page), by crossings x 100 + lap % (DNF 0), bubble-sorted, published
  every 0.25 s (0x1a3518); they stop at the finish.
- The finish order fills from the front (0x63850, not for DNF racers) and from the back for the knocked out (0x638b0
  via 0x63c40 / 0x58d20; a racer already in it keeps its place). At the player's finish (0x3fd03: the counter at
  laps + 1 and 0x52afa8 = 0) the game logic records the race time (0x83d20), then 0x3f320 adds every racer still
  out of the order by its place, then POSTABLE (0x52afa8 = 5). The time limit (0x3b920) does the same after the
  player's DNF. So the order is fixed from that frame on: later finishers find themselves already in it.
- POSTABLE's rows (0x8f140, called every frame by its update 0x8ea30 with a flag that is set only on the frame
  "POSTABLE ON" arrives; on the other frames it only colours the rows, 0xff85dde5, the player's 0xffffff00 at row
  +0x60c, and the close 0x8fa90 calls it with the flag clear to hide them). So the rows are written once, from the
  slots as they are on the player's finish frame; nothing stops the slots, the table just never reads them again.
  Per racer (row = its index in the finish order):
  - race time: 1st with crossings > laps, or laps done (counter - 1) = laps: +0x60. Else p = +0x50 / +0 (the time
    per % of the current lap): no lap done: p x 100 x laps, and the best lap +0x58 is overwritten with 100 p (the
    lap column); else +0x60 + 100 p x the laps left. Counter 0: +0x58 = 999999 and the time 999999. A zero lap %
    divides by zero (not handled).
  - 0x8f23e: down the order, a time below the one ahead's becomes that one's + 1.42 s (0x1a385c), so the estimates
    never out-run the places.
  - the cells: "%d: %s" (place, pilot; infotext 200 + pilot, scaled to fit 210) unless the craft is out of the race
    (+0x1145) without the counter at laps + 1: "%s"; out of the race: race time "DNF" (0x40e), lap time and kills
    blank; else the time (999999 -> "DNF"), the lap column +0x58 (999999 -> "DNF" in the race time cell) and the
    kills (0x1bc578 + racer). 0x8f874 prints "%d: %s race: %s best: %s" per row to the debug output.
- Argon (logic\results_rules.cpp, the pure rule, tests in results_rules_tests; logic\race.cpp, ui\results.cpp):
  App::playerFinished joins the racers still racing by place (`Race::finishByPlace`, 0x3f320) as the time limit
  does; `Race::resultOrder` = `argon::results::finishOrder` (the knocked out from the back, one already in keeps its
  place); `Race::stepStandings` adds a lap's time only up to the race's laps; POSTABLE's rows are written on its
  first draw (`ResultsTable::fillRows`: `argon::results::rowTimes` per racer, then `keepPlacesInOrder`) and printed as
  "results: n: racer r race: t best: b". Before 2026-11-16 the rows were also taken once, but without the 1.42 s
  rule, the racers still racing were ordered by place when the table was drawn (and again, later, for the score),
  the race times of drones went on adding laps after their finish, and DNF rows kept their number, lap and kills.
  Not ported: the 0.25 s place publishing (argon's places are each step's), the time before the first crossing
  in +0x60 (argon's lap timer starts at GO), the divide by zero (argon shows the time so far).
  Check: `--menu --quick-race 5 --race-go --ai-race --time-limit 100 --drive-time 120`: "results: 2: racer 3 race:
  139.77", 3: 141.19, 4: 142.61, 5: 144.03 (each the one ahead's + 1.42), 6: racer 0 DNF.
