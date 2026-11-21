# Replays: argon's port

## Argon
- `src\logic\replay.{hpp,cpp}`: parseInfo / parseIndex / parseRecording / loadRecording (the crafts' entries per
  craft, the weapon entries), nameHash, and poseAt (a craft's pose at a time). poseAt blends between the two entries
  around the time (position lerp, rotation slerp), and past the last entry dead-reckons the position; the game
  dead-reckons from the last entry only (position and rotation, 0xd06f0), which differs from the blend by at most the
  recording's thresholds (0.1 m, 0.05 per rotation element) since an entry is written as soon as the reckoning drifts
  that far.
- `src\logic\replay_race.{hpp,cpp}` (ReplayRace, App::replayRace): `--menu --replay <1..3> --race-go` loads the
  replay's track and pilots (craft i on grid slot i, crafts - 1 drones) as a Quick Race and, in place of each 60 Hz
  physics step (App::stepDrive), places every craft from the recording (GameCraft::placeFromReplay, Race::
  placeFromReplay: both state blocks, the velocity; no physics) by a clock from the countdown's start (the grid intro,
  if any, holds it; `--countdown-at 0` skips the intro). At the recording's end the front end comes back (the
  attract demo's way out, below).
  Checked by screenshots (`--drive-time 30 --nohud --shot`): replay1 and replay3 put their crafts on the racing line.
- Recording (`src\logic\replay_writer.{hpp,cpp}`, the App side `src\app\app_replays.cpp`): RecordingWriter keeps the
  game's rule (driftedFrom: the dead reckoning off by 0.1, the velocity by 1.0, a rotation element by 0.05; at most a
  frame per 0.015 s) and layout (each frame's entries in the chunk timed by the frame before, the index ended by -1,
  the rotation as shorts x 4096, controlBitsOf for +0x54; the fire bit is not set). The race is recorded each 60 Hz
  step from resetDriver (the countdown's start; the clock held by the grid intro, as ReplayRace's) until POSTABLE
  opens or 1200 s; nearly every craft is written every frame, as in the disc's replays. `--save-replay <stem>` writes
  the three files, and `tools\saves\replay.py` reads them as the disc's (checked on a bracken race: 8609 frames, 4 MB of
  events). Tests: `tests\replay_writer_tests.cpp`.
- View Replay (WINMENU / LOSEMENU, ui\results.cpp): App::startViewReplay keeps the race's standings aside
  (Race::standings), puts the crafts back on the grid (resetDriver: the drones, the pickups) and plays the race's
  recording through parseRecording, so the replay reads the game's format back; no countdown, the replay cameras, the
  HUD (App::replayHidesHud: hidden only in the attract demo and the ghost replays), the race music started over on
  on_start.wav (Audio::restartRaceMusic -> MusicModes::startOver, 0xc8e10 + mode 1; tests\music_modes_tests.cpp). The
  player's power-up levels start empty (resetPickups) and rise with its replayed pickups (signal 8 with taker 0,
  App::takeReplayedPickup -> raisePlayerPowerUps, as the game's pickup test credits the passing craft); replayed fires
  leave them. Checked: `--menu --quick-race 5 --race-go --ai-race --countdown-at 0 --drive-time 200 --view-replay
  --shot` shows the HUD with the panel lit at levels 1 / 1 / 1 (this race's caps); a real-time run (no --shot) logs
  `music: on_start.wav (mode 3)`, then bracken_music1 (twice), bracken_music2.
  At its end the menu it came from returns, the standings put back (the knocked-out drones grounded again).
  ReplayRace counts the replayed crafts' laps from their placed poses (GameCraft::followReplay, logic\lap_tally.hpp)
  and prints `replay: the player finished, <race time> (finish order: ...)` like the race's own `race end:` line.
  Checked with `--menu --quick-race 5 --race-go --ai-race --countdown-at 0 --drive-time 500 --view-replay`: the race
  and its replay both 138.48 s, order 3 5 2 4 1 0; `--quick-race 3`: both 141.27 s, 3 1 4 5 0 (with a knock-out).
- The attract demo (app_replays.cpp, logic\attract_demo.hpp): the idle clock runs in the front end (75 s, zeroed while
  any key, mouse button, pad button, stick or trigger is held; not on FE INTROSCREENS, FE MESSAGEBOX, FE OPTIONS
  CREDITS or the loading screen), then replay1, 2, 3 in turn as a Quick Race behind the loading screen, which goes on
  without waiting for A; no HUD, no pause, the replay cameras. START, A, B, X, Y (keys Enter, Space, Esc, Backspace, X, Y,
  P) or the replay's end go back to the front end on the page that was left (FE TITLE stays FE TITLE). Tests: `--menu
  --menu-page 6 --attract-after 1` (the idle time shortened), `--menu --replay n --race-go --countdown-at 0
  --drive-time 220` (the whole replay, then FE MAIN).
- The pause menu's ghost replays (ui\pause.cpp): Replay Loaded / Session Ghost close the pause and replay that ghost's
  lap alone from its start (ReplayRace::startGhostLap: argon's ghost samples as craft entries; the player's craft
  hidden and placed from them for the camera, the ghost craft shown at the same pose, the other ghost hidden), no HUD,
  the replay cameras; at the lap's end the run restarts from the line (restartRace). Test: `--menu --time-attack
  --quick-race 5 --race-go --ai-race --countdown-at 0 --drive-time 150 --ghost-replay 1 47`.
- View Replay and the ghost replays also end on START / A / B / X / Y here (not checked in the game).
- The weapon entries (`src\logic\replay_weapons.{hpp,cpp}` builds and reads them, `src\app\app_replay_weapons.cpp`):
  recorded as the race's weapons act: fires (7, fireColour), launches (2, launchFromVolley), each projectile's state
  every step (kind 1), done (3), impacts by kind (4, from finishShot, argon's 0x65f80, and the bounces), every
  craft's shield when it changed in the step (1, the killer in +0x1f when it is knocked out) and the pickups (8 / 9).
  Played as 0x641f0 does: a launch puts the projectile on its bone, its states then place it (no flight, no hits), an
  impact shows its bursts, the shield entries set the shields (and knock out), a fire raises a shield, the pickups go
  and come back; the hits themselves are not replayed (Weapons::applyHit does nothing during a replay). The disc's replays
  play theirs the same way (replay1: 57 fires, 113 launches, 454 impacts, 143 shield values, 2352 states, 241
  pickups; craft 2, which never finishes, is knocked out). Not written: 6 (the power level, HUD only) and 0xb.
- View Replay restarts the effects' clock (simClock: the shields' ages, the hits' effects, the particles) where the
  race had it and moves it from the same step (RaceRecording::Timing), so the replay sees the race's times to the float.
  Checked with `--menu --quick-race <t> --race-go --ai-race --autofire --countdown-at 0 --drive-time 520
  --view-replay` on tracks 0, 1, 2, 4, 6 (2026-11-22; races of 173.65, 161.75, 162.50, 122.31 and 200.40 s): the race
  and its replay give the same weapon tally (fires, launches, done, impacts, shield values, projectile states,
  pickups and an FNV hash of their sequence) and the same shields at the end, to the decimal (`replay: weapons
  recorded / played`, `replay: shields at the race's / replay's end`); none of these races knocks a craft out. An
  earlier record of this check (a drone knocked out on track 2, the player on track 6) ran without `--menu`, which
  then drove the editor's bracken each time (see [argon/editor.md](../argon/editor.md) "Command line").
- The replayed control bits (craft_replay_inputs 0x61c60): GameCraft::setReplayControls puts them back as the slots (0
  / 100, controlSlotsOf) and the brake; the jets and the flaps then follow as a flying craft's (App::showReplayedControls).
  In the first 3 s of the clock a craft eases 10 % a frame to its recorded position (easedStartPosition), not in the
  ghost replays (Time Attack).
- The replay cameras (`src\logic\replay_director.{hpp,cpp}`, ReplayDirector): the decoded director above, the four
  camera kinds as classes; App::replayCameraShot feeds it the player's craft (its lap % from the AI route) and shows
  its shot through the TV camera view while a replay plays (argon's own TV cameras otherwise); camera changes are
  printed as `replay camera <n>: <type> "<name>" at <clock> s`. The KERB shake is KerbShake (fed every racer's position
  and speed, App::passingCrafts). Tests: `tests\replay_director_tests.cpp`. A still of a KERB camera
  (`--drive-time 167 --view-replay --shot` on bracken: "down before jump" as the pack flies over) cannot show the
  sub-degree jitter; not compared with the game.
- Not ported yet: signal 6 (nothing visible, above) and 0xb (the retaliation's own effect; its projectiles are
  replayed), the fire bit in the control bits.
- To compare with the game (xemu, the USA ISO): the attract demo's first seconds (the 3 s easing on the grid), whether
  the crafts' flaps / jets follow the replayed control bits, the replay cameras' cuts and framing (the order on
  bracken: Stadium1, the ship camera "Nose looking backwards", over first water, left jump corner, ...), whether the
  attract demo shows the countdown models ("3 2 1 GO" are drawn here), View Replay's HUD (shown, as decoded) and whether
  its power-up panel fills with the replayed pickups and stays full, its music (on_start.wav from the replay's start),
  whether it shows the countdown, the KERB cameras' shake (bracken "down before jump", flames "Start line"), whether a
  button ends View Replay or a ghost replay, and the replayed projectiles, impacts and pickups.
