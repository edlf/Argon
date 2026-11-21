# Front end: game modes and Time Attack

## Game modes (0x52ccfc)
- The race's game-logic object (0x7f222, jump table 0x7f384; all on the base ctor 0x3ac40, vtable slot 0 init,
  1 update, 2 shutdown, 10 the crafts): 0 Tournament vtable 0x1a1bd8, 1 Quick Race 0x1a18f8, 2 CAttractDemo 0x1a1a0c,
  3 Time Attack 0x1a16f8 (0xd4c4 bytes, the others 0xd39c), 4 0x1a197c (multiplayer). The front end reads
  0x52ccfc directly (FE CHARSELECT, FE SPEEDSELECT, LOADER, the race end).
- Argon (logic\game_logic.hpp): `TournamentLogic`, `QuickRaceLogic`, `TimeAttackLogic` on `GameLogic`, one object
  per mode in `GameModes` (`App::modes`, logic\game_logic.hpp), selected by FE MAIN (and the tests' `--time-attack` / `--tourney-race`). The race, the
  HUD and the pages ask `App::logic()`: the next page (`characterPicked`, `speedPicked`), the rules (`hasOpponents`,
  `hasPickups`, `hasWeapons`, `scoresPoints`, `hasLapLimit`, `hasGridIntro`, `hasGhosts`), the scores (`pilot`,
  `highScores`, `earnCash`) and the race end (`hasUpgrades`, `paysCash`, `raceFinished`). The objects live as long
  as argon (the game builds one per race); the race state stays in App.

## Time Attack (game mode 3, 0x52ccfc)
FE MAIN row 1 (fe 0x7a) -> FE CHARSELECT -> FE COURSE SELECT -> FE SPEEDSELECT -> FE GHOSTSELECT -> LOADER -> the race.
- FE SPEEDSELECT: the right hint 0x32 (else 0x3d, enter 0x34200); A goes to FE GHOSTSELECT (update 0x33fa0).
- LOADER: the hint is race_strings 0x64a + 0x1bbc28 (Y: next, wraps after 15; loader_next_hint 0x81be0, draw 0x8096a).
- The race (checks of mode 3): no opponents (the grid 0x79250 adds none in modes 2 / 3 / 4); no weapons (weapon_launch
  0x64e20, the weapon updates 0x641a0 / 0x646e0 / 0x64870 / 0x649f0); no pickups (pickups_draw 0x8fbc0); no race
  points (score_award 0x83f00, score_lap 0x83d80, the air award in craft_hover 0x5e6d0); no lap limit (race_manager_update
  0x62ba0 counts laps and adds the times on past g_race_laps: the race never finishes, it is left through the pause
  menu); no grid intro (race_intro_start 0x43d60) and the race clock is not reset at the countdown (race_clock_reset
  0x3f1d0, modes 3 / 4). The player's lap turbo (race_craft_lap 0x3d5a0) and the lap records (g_track_records + (speed
  + track x 5) x 0x18: best lap and the profile name, all modes) are as in the other modes.
- HUD: laps "%d[F:0] / --" (gamehud_update 0x496f0); hidden: the power-up panel +0x3950 and the score +0x66a4 /
  +0x68b0 (neon_set_hud_mode_2 0x49440), the shield bar and its value (gamehud_render 0x4a100), the position list; the
  minimap without its 50 px offset (hud_minimap_draw 0x4f090).
- Pause menu: Replay Loaded Ghost / Replay Session Ghost shown (0x52afaf / 0x52afae, pausemenu_update 0x898e0); Main
  Menu / Track Select go to FE GHOSTSAVE first with a new ghost (0x459a8e; 0x8b5b0 / 0x8b630).
- Ghosts (0x429c0, each frame in Time Attack): the player's inputs and position are written to the lap's event buffer
  (craft_record_inputs, CEventBuffer::WriteEntry); at a lap's end (0x439ff6) a lap faster than the best (+0xd414 <
  +0xd418) is copied to the session ghost (0xd9c90) and 0x52afae set. Two ghost crafts replay from the lap's start:
  the loaded ghost (+0xce74, 0x52afaf) and the session ghost (+0xce70), positions interpolated from the buffer 0x442e90
  (0xd06f0) and the controls from the bitmask +0x838 (craft_replay_inputs 0x61c60); +0x1129 hides an unused one. The
  other slots get +0x1144 / +0x188 / +0x20+0x9c in mode 3 (0x3dc70): +0x188 = no physics step (0xcf971 / 0xd003f),
  +0x20 -> +0x9c = 0 the collision body is not collidable (read at 0xcfbf1 / 0xd0290 / 0xd3ca6), +0x1144 no water
  splash; none of them changes the draw.
- Turbo (0x429c0, Time Attack's update 0x43220 only, vtable 0x1a16f8 [1]; the other modes get race_craft_lap's
  1.5 + 0.5 x level per counted lap, the start crossing included, [physics/turbo-flaps-cameras.md](../physics/turbo-flaps-cameras.md) "Turbo"): a crossing of the lap line with more than 1 s on the lap clock raises 0x439ff6; the
  next pass sets (not adds: it overrides race_craft_lap's +1.5 + 0.5 x level) the turbo to (1.5 + 0.5 x Turbo level)
  x g_race_laps (2 / 3 / 5 laps by the speed class, [physics/turbo-flaps-cameras.md](../physics/turbo-flaps-cameras.md) "Turbo"). The first crossing after GO counts too, so a
  race starts with a full turbo and each lap refills it.
  Argon: `App::stepLapLine` (app_lapline.cpp) on `GameCraft::lineCrossings`, then `TimeAttackLogic::lapLineCrossed`.
- The lap line (race_manager_update 0x62ba0): a racer's lap % is its current AI node's (the route table 0xe3c90 at
  0x42edd8 + 0x684 per node: the start node 0x1b9bd0 0.1 %, the node before it 99 %, the others their distance along
  the routes, averaged over the routes through them); a drop of 90 or more is a crossing (a rise of 90 undoes one,
  +0x57c |= 2; a counted lap sets +0x57c |= 1, the bit 0x429c0 waits for). The current node (+0x540) changes when the
  craft passes a transition plane of its node's hull (table 0x1b9bd4: +8 forward / +0x88 back planes, from the
  config\ai_<t>.txt <hulls>: <transplane> = n x y z + the node it leads to, the plane through the node + n with normal
  n; the craft has passed it when (pos - node - n) . n > 0). The files' planes lie halfway to the next node (bracken:
  node 51 -> 0, |n| 55.87 of 111.7 m), so the line is halfway between the last node and the start node: 19 to 126 m
  ahead of the pole on the 18 tracks (bracken 44 m). Argon: craftsim.cpp (a drive on bracken crosses at 47 m);
  the turbo's 1 s check passes on the first crossing (the game's clock +0xd330 holds the countdown too).
- FE GHOSTSELECT "Select Ghost" (fe 0x348; vtable 0x19e0f8: build 0x234d0, input 0x23b40, update 0x239c0, draw
  0x238d0, messages 0x241b0, list 0x24c90): frames (52, 100) 536 x 40 (swapped; its header text stays empty) and
  (52, 140) 536 x 160 (draw 0x238d0, as FE GHOSTMANAGE); five rows font 0 at (62, 170 + 20 i); icons\up.dds at
  (62 + 20, 148) while rows are scrolled off the top (+0x1420 > 0), icons\down.dds at (82, 279) while more of the
  track's ghosts follow the five shown, both 40 x 20 (build 0x234d0, list 0x24c90; ported). The rows (0x24c90) are the save device's ghost list (0x459fbc: name, character
  +0x208, track +0x20c, next +0x210) for this track: "No Loaded Ghost" (fe 0x349, character -1) first, then each
  ghost named by its character (infotext 200 + id; "Invalid ghost." 0x1e0 for a bad one). Up / down move and
  scroll (+0x141c the row, +0x1420 the first shown; the count 0xc1540), A picks: the first row empties
  z:\eventbuffer\preloaded.index / .events (no loaded ghost), the others load the ghost ("Unable to load ghost."
  0x369); then LOADER (update: sequence 7). B: FE SPEEDSELECT (sequence 4); Y: "Are you sure you want to launch the
  Xbox Dashboard?" (0x34b; footer "Dashboard - [BUTX]" 0x41). Ticker 0x34d "Select which ghost you want to race
  against.", hints Back (0x33) / Select (0x32). Opening checks the save device for 0x100000 bytes free (0x34a /
  0x34c otherwise).
- FE GHOSTSAVE "Save Ghost" (fe 0x35c; vtable 0x19de80: update 0x22780, messages 0x228d0, built by the app's start
  0x77da2): the page is message boxes. Without a valid lap (0x52ac27) it goes on at once; else it looks for a ghost
  of this character (0x52cdc0) and track (0x52ccf8). None: "Do you want to save your ghost from that session?"
  (0x1a4, kind 5); one: "A ghost already exists for this character and track. ..." (0x1a5, kind 6 "Yes, replace
  the old ghost" 0x1a7 / "No, keep the old ghost" 0x1a8). Saving shows "Saving ghost. ..." (0x1fc, kind 0xb; "Could
  not save ghost." 0x35f); not saving asks "Are you sure you want to continue without saving your ghost?" (0x1a9) /
  "Are you sure?" (0x1a6), No back to the first question. Then FE MAIN (0x1a3120). Other texts: 0x1aa "Lap time
  was too slow to save a ghost.", 0x368 "You can save your ghost from that session." (where they show: not checked).
- Not checked: FUN_00070880 (mode 3: A -> 0x52abe8 = 1 and a GAMELOGIC 5 message, a restart?).
- Argon: `GameModes::timeAttack()`; the route (ui\menu.cpp, ui\speed.cpp: FE GHOSTSELECT not ported, straight to the
  loading screen), the hints (ui\loader.cpp `loaderHint`), the race rules (logic\race.cpp, app_pickups.cpp,
  weapons.cpp, app_results.cpp, app_loader.cpp), the HUD (hud\game_hud.cpp). `--time-attack` with `--quick-race` (tests).
- Ghost replay (0xd06f0): no physics; the craft is placed from its last event by dead reckoning (position + velocity
  x dt, orientation turned by the angular velocity x dt, current and previous state alike); craft_replay_inputs
  0x61c60 always takes that path in mode 3 (else it eases towards the recorded state for 1 s first) and sets the
  thrusters / airbrakes from the bits +0x838 (visual). +0x1144 marks a ghost: craft_water 0x61340 plays no splash
  for it. Look: a ghost is drawn as any craft, opaque (craft draw 0x59bc0, the same render list path), and casts
  a shadow (the shadow loop 0x93871 skips only +0x1145 / +0x1129 crafts); checked live in xemu
  (tools\xemu\ghost_probe.py: same bone flags / tints / intensities as the player's craft; tools\xemu\craft_drawstate.py)
  and by eye. The pause menu's Replay Loaded / Session Ghost (0x8b560) close the pause and send GAMELOGIC 3 / 4:
  Time Attack replays that ghost's lap alone from the line (the "preloaded" / "fastest" buffer, no HUD, the replay
  cameras), then starts the run again ([replays/users.md](../replays/users.md) "Users"; 0x448e50 is only the pause menu's state).
- Argon (logic\time_attack.cpp, ghost.hpp): `TimeAttackLogic` holds the lap being recorded, the session ghost, the
  loaded ghost and the two ghost crafts, as the game's larger Time Attack object does; FE GHOSTSELECT hands it the
  loaded lap (`useLoadedGhost`); leaving the race hands FE GHOSTSAVE the session ghost (`takeSessionGhost`,
  `GhostSavePage::open(next, lap)`). The session ghost: from the first line crossing every race step's pose and jet
  size are recorded; a finished lap faster than the session ghost replaces it; the ghost is the player's craft model at the ghost lap's pose for this lap's time (blended between samples and steps), hidden past its end,
  drawn solid without a shadow (not compared with the game). A new race (Restart, Track Select) clears it. The
  pause menu shows Replay Session Ghost, which replays the lap (app_replays.cpp, [replays/argon.md](../replays/argon.md) "Argon"). Tests:
  tests\menu_tests.cpp (sampling, the best lap, the file).
- Argon, saved ghosts (saves\ghost_library.cpp, ui\ghostselect.cpp, ui\ghostsave.cpp, ui\records.cpp): one file per
  character and track, <game>\ghosts\<track>_<pilot>.ghost (ghost.hpp: "QRGH", version 1, track, character, speed,
  lap time, the samples), shared by all profiles as the game's save device list. FE GHOSTSELECT (page 23) after
  FE SPEEDSELECT in Time Attack: "No Loaded Ghost" and this track's ghosts (skipped when the track has none: straight on to
  the loading screen); the pick is the loaded ghost, replayed
  with the session ghost in its own character's craft (Restart keeps it, a new FE GHOSTSELECT replaces it); the
  pause menu then shows Replay Loaded Ghost. Leaving the race through the pause menu's Main Menu / Track
  Select with a session ghost goes through FE GHOSTSAVE (page 24; the message box kind 4 = the game's kind 6), then
  FE MAIN / FE COURSE SELECT. Ghost Data (FE GHOSTMANAGE): see "Options sub-pages". Not ported: the Dashboard and
  the save device's messages.
