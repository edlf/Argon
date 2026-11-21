# Front end: multiplayer

## Multiplayer (game mode 4)
- FE MAIN's Multiplayer (0x28e90, needs two controllers): FE CHARSELECT in mode 4 (0x78d30). Each controller (pad p,
  the input event's +4) has its own cursor (+0x1118 + p x 4): START / A joins (0x17640: the next free player slot in
  0x52abec[4] takes the pad, "AppState - setting num players" 0x52cd8c + 1), then moves over the 16 tiles and A picks
  an unlocked character (+0x2b09 bit p, +0x2b0a[p], 0x52cdc0[slot] = character). Once every joined player has picked
  and they are two at least: the craft count = the players (0x78d60), the players ordered by pad (0x174a0), FE COURSE
  SELECT. B unpicks; before a pick it leaves for FE MAIN and drops every player (0x52abec = -1). No X / Y pages.
- Draw 0x178a0 with mode 4: per controller a gradient frame 124 x 124 at y 112 (x 60 + 2 + 132 p) with "Please
  connect a controller" (fe 0xab), "Press START to join!" (0xa7, blinking) or the player's character (3D in the game);
  the grid as single player, a tile with a player on it highlighted, filled once all on it picked, the players'
  numbers "1".."4" at its corners; the instructions (fe 0xa8) in the ticker.
- The race (vtable 0x1a197c, gl_multiplayer.cpp; race setup 0x40fb0): hud0..hud3 and camera0..camera3 with the
  views of 0x7efd0 (one: 640 x 480; two: top / bottom halves; three / four: the quarters, three leaving the bottom
  right empty). No AI on the grid (0x79250 skips mode 4). Laps: the Multiplayer Laps option 0x10 (Default keeps the
  race's, 3 / 5 / 7 / 9; 0x3b0b0). The race setup's caps and Shield level come from the speed / kind table for every
  craft, slot 0 too (0x3f3d0 skips 0x3f650). The update (0x41490) runs race_craft_lap for every craft (each lap's
  turbo grant).
- HUD per player (gamehud_layout 0x45270 / render 0x4a100 with +0x5594 != 1): the frame keeps 52 / 36 from the
  screen's edges and 12 from the other views; no time block, score, speed read-out, minimap, position list, fan or
  needles; the power-up panel 32 right at half size; the shield bar at B - 32, 70 + S x 70 / 180 long (label at y -
  19, value at L + 5); the turbo "%.1fs" in a 72 x 20 box at (R - 76, B - 32) under "Turbo" right-aligned.
- The end (0x41de0 / 0x620e0): MULTIPLAYERENDMENU (0x52afa8 = 7) once the finish order holds every player, or after a
  knock-out with one left (it joins the order, "By Default" 0x515). Page (multiplayerend.cpp, 0x85f18..0x88100):
  "Race Over" (0x41c), Race Again (0x3fa: GAMELOGIC message 5, 0x880d0) / Track Select (0x3fb) / Main Menu (0x3fc:
  state 8, 0x88000) at (320, 100 + 35 n) with "Are you sure?" Yes / No, "Select - [BUTA]" (0x408); the table
  "Position" (0x271), "Race Time" (0x294), "Lap Time" (0x295), "Wins" (0x429), a row per player ("Player n",
  infotext 0x218 + n; "DNF" 0x40e). Any player's pad drives it (0x3bf60 state 7).
- Argon: game mode (logic\game_logic.hpp MultiplayerLogic, logic\multiplayer_rules.hpp), the views
  (logic\split_screen.hpp, App::raceViews; per-view scene constants and sprite buffers), players 2..4 fly the first
  drones on their own gamepads (Race::stepHuman, App::readPlayerPad), a GameHud per player, ui\multiplayer_select.cpp
  and ui\multiplayer_end.cpp. Tests: `--multiplayer n` (mode 4, n players; with `--ai-race` the AI flies every seat),
  `--split n` (the views over a Quick Race's first racers).
- FE CHARSELECT's boxes (0x178a0): each joined player's character in 3D from a camera on its `<pilot>_skel_head`
  bone (0xb5190 finds it by name): the eye 14 out (Kumiko 21) and 1.4 up (2.1), swinging 90 +- 30 degrees with
  sin(time x 10 deg) from when the box showed that character (+0x1148), fov 36 degrees, the 124 x 124 box (x 62 +
  128 n, y 112). Argon: FeScene::boxed, MultiplayerSelect::boxScenes, drawn over the frames (FeShellDrawing::drawBoxModels).
- LOADER (0x81052): a row per player still to press A on their own controller, at (margin + 20, 234 + 40 n), font 1
  x 0.7 in the status colour over a highlighted panel: "Player n - Press [BUTA] to race." (race 0x537 + port), or
  "Player n - Reconnect controller." + "port n" (0x53c / 0x544 + port); the race starts once all have pressed A.
- Controllers by port (GLFW joystick id, the Xbox's ports 0..3): a player's port losing its controller opens the
  controller message for it, strings 0x44c + port ("Please insert a controller into port n, and press START."),
  kind 3: only that port closes it (0x30a70).
- MULTIPLAYERENDMENU's layout (0x85f10 / rows 0x86e70): title right-aligned at (568, 36); columns from the right
  margin 115 / 115 / 95 wide (0x1b3ccc), table top 0x1b3cc8 = 220, headers at top + 5, rows centred on top + 16 +
  32 n from n = 1; "n: [CHARc] Player p" (a knocked-out player without the place) scaled to fit 210, the race time
  (DNF / By Default), the lap time, the wins (0x52af88).
- The race: no full-screen clear (each camera clears its viewport, 0x93717 / 0xae3c3), so three players leave the
  bottom right quarter as it was (black here). Each player's lock lists and crosshairs (0x6c420 / 0x4e7f0) and the
  nemesis marker are drawn in their own view. Sound: in mode 4 every craft's engine buffer sits on the listener
  (0xc5510: SetPosition(0, 0, 0)), so each player's engine is heard centred on its own curve (argon: the normal
  set, engine\, for players 2..4; h_engine for player 1 as before).
- To compare in xemu: the box camera's framing, the loader rows over the hint box, the end table, whether players
  2..4's crafts load h_engine too, the turbo sound for players 2..4 (argon plays player 1's only).

## Still open
What is still open for the front end is in [argon/todo.md](../argon/todo.md) ("To compare with the game", "Front end"). Also: the haze's
exact tint (a little warmer in the centre and a cooler top bar than a screenshot of the game).
