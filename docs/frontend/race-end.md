# Front end: race end and the pause menu

## Race end (Quick Race)
- The race manager (0x596db0; update 0x62ba0) keeps per racer a slot (+0x548 + id x 0x88): +0x10 the lap counter
  (starts at 1), +0x60 the race time, ... ([physics/standings.md](../physics/standings.md) "Standings"); the finish order at +0x514 (0x5972c4), filled from the front when a
  racer's lap counter reaches laps + 1 (0x63850; the lap count 0x52cdd8) and from the back for the knocked-out
  (0x638b0, "dnf" to its HUD). 0x63820 gives a racer's place in it (1-based, -1 not finished).
- Game logic (0x3fba0 / 0x41490 per mode): when the player's lap counter reaches laps + 1 and 0x52afa8 = 0, the race
  time is recorded (0x83d20), POSTABLE opens (0x52afa8 = 5) and the HUD is set to "set none"; the race runs on
  behind it. The 20-minute limit (0x3b920, modes 0 / 1; Time Attack has its own clock check): once the race clock
  (0x50e430 + 0x50e434, reset at the race setup 0x3d34f / 0x3e870) reaches 1200 s (0x1b9790) while 0x52afa8 = 0
  and neither 0x52abe8 nor 0x52abe9 is set: the player's race time stat is 9999.99 (0x83d20), the player is out
  (+0x1145) and, not having finished, DNF (racer +0x6a, 0x638b0 from the back; 0x63c40 and again inline), every racer
  not yet in the finish order joins it by its place now (0x3f320 -> 0x63850), then the HUD "set none", POSTABLE
  (0x52afa8 = 5). A knocked-out player goes to LOSEMENU without POSTABLE ([weapons/explosions.md](../weapons/explosions.md) "Knock-out"). The dispatcher 0x3bf60
  passes the pad on: in state 5 A to POSTABLE; in 1 / 2 up / down / A (messages 5 / 6 / 9) to WINMENU / LOSEMENU.
- POSTABLE (postable.cpp; vtable 0x19d018: init 0x8dbb0, update 0x8ea30, rows 0x8f140, close 0x8fa90): title
  "Results" (race 0x428) font 1 right-aligned at (568, 36); the player's place ("1st".."6th", 0x25e + place) centred
  at (320, 90) and "Congratulations!" (0x4c9, first) or "Try Again!" (0x410) at (320, 120); headers orange
  0xffff8000 at y 160 + 5: "Position/Character" (0x26e) at 67, "Race Time" (0x294) at 568 - 285 + 5, "Lap Time"
  (0x295) at 568 - 170 + 5, "Kills" (0x41b) centred at 568 - 55 + 27.5 (column widths 0x1b3cb4..: 115, 115, 55;
  the time headers scaled to fit 108); one row per racer 30 apart in finish order: "%d: %s" (place: infotext 200 +
  pilot; knocked-out racers without the number), scaled to fit 210; the race time (racers still racing, 0x8f140:
  p = the current lap's time so far / the fraction of it done (slot +0x50 / +0), then time of the laps done (+0x60)
  + p x laps left, or p x laps before the first lap, which also writes p as the lap time; no lap started: 999999 ->
  "DNF" 0x40e; a time below the place ahead's becomes that one's + 1.42 s; a racer out of the race, craft +0x1145:
  knocked out or the player at the time limit, shows "DNF" with blank lap time and kills), the lap time (the best,
  slot +0x58), the kills (0x1bc578 + racer); hint
  "Press [BUTA] to continue." (0x406) at (568, 421). A (sound 0x18): 0x8d610 opens WINMENU (0x52afa8 = 1) when the
  player finished first and was not knocked out (0x597362), else LOSEMENU (2).
- WINMENU (init 0xa01c8) / LOSEMENU (init 0x845b0, vtable 0x19c820: update 0x84b30, items 0x85a50, up / down 0x85760
  / 0x858e0, pick 0x85660, confirm 0x85bf0): title "You Won" (0x3f2) / "You Lost" (0x3e8) font 1 right-aligned at
  (568, 36); 4 items font 1 x 0.7 centred on 320 at y 151 + 35 n: Quick Race Race Again (0x3fa), View Replay
  (0x3eb), Track Select (0x3fb), Main Menu (0x3fc); Tournament: Upgrade Menu (0x528) / Next Race (0x3f3), Race
  Again, View Replay, Main Menu / Save and Quit (0x40f) / Continue (0x3ed); "You have not upgraded your craft."
  (0x536) at (320, 116); hint "Select - [BUTA]" (0x408). Race Again, Track Select and Main Menu ask "Are you sure?"
  (0x40c) Yes (0x40a) / No (0x40b) first; Yes shows "Saving Race" for 1 s with the music fading out (to -9600 mB, [audio/music.md](../audio/music.md) "Music fades") before the item runs (0x84b30). Drawn, as
  POSTABLE, SCOREMENU and POINTSMENU, over pause_background (0x8e808, 0x84b24, 0xa0744) every frame: it blurs the echo
  the race scene copies each frame, so the race runs on behind the pages, blurred (held only in the pause, where the
  race stops), with the menu bars and logo. The race is seen through the replay cameras once the player has finished
  (seen on the console; the switch is not traced: no "replay on" reaches camera0 at the finish, the senders are View
  Replay 0x3ff00, the attract demo and Time Attack).
- Argon (logic\race.cpp `Race`, app_results.cpp): the finish order, race times and kills are counted by argon; the player's finish
  opens the table, the AI pilot drives the player's craft on behind it (the game's player craft after the finish
  is not checked), the HUD is hidden; the race view follows the replay cameras' director (App::followsRaceEndCameras:
  single player, not knocked out) and the pages blur the live frame each frame (RaceMenus::drawRaceBlur). Race Again restarts from the grid, Track Select opens FE COURSE SELECT, Main
  Menu FE MAIN. The 20-minute limit (App::endRaceAtTimeLimit; argon's race clock starts at GO, not at the
  setup; test `--time-limit <s>`) and the knocked-out path (app_knockout.cpp, test `--knockout-at <s>`) are ported;
  the table estimates the race time of a racer short of the line (0x8f140). The table's rows are written once, on
  its first frame, as the game's: 0x8ea30 calls 0x8f140 every frame but lets it write the rows only on the frame
  "POSTABLE ON" arrives (later frames only colour them); the race slots run on ([physics/standings.md](../physics/standings.md) "Standings": the order
  fixed at the player's finish by 0x3f320, the estimates, the 1.42 s rule, the DNF rows). View Replay plays the race's recording from the grid and comes back to the menu ([replays/argon.md](../replays/argon.md) "Argon";
  test `--view-replay`). The music fades and the second of "Saving Race" on leaving SCOREMENU / POINTSMENU / UPGRADEMENU / WINMENU /
  LOSEMENU are ported ([audio/music.md](../audio/music.md) "Music fades"; test `--finish-leave <s>`). Not ported: Tournament's items, the table's fonts for the place / "Congratulations!" lines (font 1 x 0.7 here: not
  checked), the confirm page's positions (151 / 221 / 256 here: not decoded). Test flags: `--finish 0..2` (table /
  WINMENU / LOSEMENU) and `--ai-race` with `--quick-race <id> --race-go`.
- Races started from the menus draw none of the editor's markings (start grid, AI route, power-ups, emitters,
  route issues: EditorSession::active, set only when the editor starts: Game Demos or a start without the menus); the
  editor keeps them.
- Fix: the game's right edge constant 0x1a36c8 is 630, so right-aligned texts sit at 630 - 52 - 10 = 568 (the
  pause menu and Track Data used 578 before).

## Pause menu (PAUSEMENU, pausemenu.cpp)
- Text scale: the race's menus (PAUSEMENU, POSTABLE, SCOREMENU, POINTSMENU, WINMENU / LOSEMENU, UPGRADEMENU) are
  game-logic hud groups and draw at the
  elements' own scales (titles 1.0, items 0.7, the hint "Select - [BUTA]" font 0 x 1.0), without the front-end
  pages' 0.75 text factor; checked against xemu screenshots of Paused and Upgrade (argon's were 1 / 0.75 small);
  the result pages too (compared with xemu pictures of Results and Race Scores).
- Result pages as the game shows them (argon ui\results.cpp / ui\score.cpp): POSTABLE, SCOREMENU and POINTSMENU over
  the race frame blurred once (pause_background) with the menu bars and logo; Results: "1st" and "Congratulations!" /
  "Try Again!" font 1 x 1.0 yellow, the table as a grid of gradient-framed cells (0xc2310: the header row the
  reddish variant at y 160, a row per racer 30 high; columns 62 / 283 / 398 / 513 / 568), the player's row yellow;
  Race Scores: "Race Score:", "Score this race =" and "Previous High Score =" with their values yellow, NEW CASH font 1
  x 1.0 yellow; the footers right-aligned at 568, y 397 ("Press [BUTA] to continue.", "Select - [BUTA]") and 421
  (the second line, "Show score breakdown - [BUTX]"), also in PAUSEMENU, WINMENU / LOSEMENU and UPGRADEMENU.
- A game-logic hud group (vtable 0x19c5e0: ctor 0x88930, init 0x88ab0, update / messages 0x898e0, draw 0x89860),
  driven by messages: "PAUSE ON" (Start) stops every pad's rumble, sets the vibration flags 0x52c8c0, the cursor on
  Continue (+0x60c = 0), page +0x610 = 0, item positions 0x8be20; "PAUSE OFF" resumes; "PAUSE QUIT" leaves (POSTABLE).
  Cursor messages 5 / 6 (0x8bad0 / 0x8bc80) step through each page's items, wrapping; 7 / 8 left / right; 9 A
  (pad 0x10 or 0x100); 0xb resumes from any page (no sender found). The race's pad handler (gamelogic_post_race_input
  0x3bf60) sends only 5..9, and Start toggles the pause (0x721f0 -> 0x782d0, only the pad that paused, 0x52afa4; no
  sound): B is never passed on, so it does nothing in the pause menu. Texts are race_strings.
- Sounds (sound_play_ui 0xc3ca0 (id, 0x448f58, 0), the race's generic list): up / down 0x17 MENUNAVIGATE on every
  press (0x8bad0 / 0x8bc80, after the move); left / right on a volume row 0x17 only
  when the entry moved inside 0..7 (0x89a69 / 0x89b96: at 0 or 7, and on any other row, silent); A 0x18 MENUSELECT
  after every item but the volume rows (0x89d6c; 7..9 take the switch's default, silent); message 0xb 0x18. No sound
  on Start.
- "Saving Race": a volume step sets the profile's dirty flag 0x584339. Continue (0x89bd6), the ghost replays
  (0x8b560), Yes on Restart / Track Select / Main Menu (0x8b6f0 / 0x8b630 / 0x8b5b0) with it set start the save:
  +0x1d4 = 1, +0x618 = the clock 0x292d58. While +0x1d4 the message loop drops every message; each frame
  (0x89f42) clears 0x584339, shows "Saving Race" (0x520, element +0x1274, font 0, flags 0x1020: left-aligned at
  (margin 52 + 10, 421), the line of the hint) and hides the hint (+0x1480) and the hud cursor (+0x28fc: it draws
  nothing, the selected item keeps its highlight); after 0.5 s (0x1a3504) the profile is written (0xbfac0 on
  0x459a88, once a race: +0x61c, cleared only by init 0x88ab0), after 1 s (0x1a3454) "Saving Race" hides and +0x1d4 =
  0. Only then the item's flags run: +0x628 resume (0x8b470, PAUSE OFF), +0x624 / +0x625 / +0x626 / +0x627 the
  GAMELOGIC messages 0 / 5 / 3 / 4. The other items call 0x782d0 (the gamelogic's pause flag) at once, but PAUSE OFF
  (+0x61d) also waits, so the menu stays up for the second. Start resumes without a save (the flag stays set).
- Controller pulled out (pausemenu_update 0x8a7f0..0x8b0a6, not in game mode 2; nothing after Track Select / Main
  Menu, 0x52ac01): each frame, for each local player (0x52abec[i] != -1) whose pad is gone (0x298d60[port] +0 = 0)
  and whose message is not up (+0x61e + i): "insert on" to hud<i> (the HUD's text +0x6ea0: race 0x548 + port
  "Please reconnect controller into port n, and press START.", font 0, flags 0x102, white, set in 0x45ef0; the
  layout 0x45403 centres it across the view's safe width, (B - T) / 2 below the view's top, wrapping to that width)
  and, while the race runs (g_post_race_state 0), PAUSE ON with 0x52afa4 = i; 0x52abfc[i] = 1. While any player is
  out or waiting (0x52abfc) the pause pages are forced hidden (+0x614..+0x617 = 0: no blur, the race frame and HUD
  stay with the message), and the race input (0x3bf60) passes no press on; there START on a waiting player's pad
  (+0x89 & 1, back in its port) clears 0x52abfc[i] (and 0x721f0 toggles the pause). A player neither out nor waiting
  gets "insert off"; when that leaves nobody out or waiting, PAUSE ON again: the pause menu on Continue. The front
  end's own string fe 7 / race 0x3f9 ("Please reconnect the controller, and press START to continue.") is not used
  in the race: no reference to 0x3f9 in the XBE; the loader shows 0x540 + port (0x80da6).
- Main page (+0x615): items 0 Continue (0x3ed), 5 Options (0x3f0), 10 Restart (0x3f1), 1 Replay Loaded Ghost (0x3ee,
  Time Attack with 0x52afaf), 2 Replay Session Ghost (0x3ef, 0x52afae), 3 Track Select (0x3fb, not in Tournament), 4
  Main Menu (0x3fc), shown top to bottom in that order at x 320 (centred), y 116 + 35 n. Title "Paused" (0x3ec)
  font 1 right-aligned at (568, 36); the items font 1 x 0.7; hint "Select - [BUTA]" (0x408) font 0 right-aligned at
  (568, 421); "Saving Race" (0x520) at the left of that line while saving. Every item has the panel flag
  (+0x1cc |= 2: a panel behind the text, 0xad620); the hud cursor (+0x28fc, 0xacbb0: mode +0x1d4 = 0, +0x1d8 8, +0x1dc
  0.2, margins +0x1e0 4 / +0x1e4 0) draws nothing and only marks the selected item (0xacc30), whose panel is then
  highlighted and filled ("The hud cursor"). "Are you sure?" and the title have no panel.
- Options page (+0x610 = 1, 0x8b6c0): 6 Exit (0x3f5, back to the main page on Continue, 0x8b7d0), 7 SFX Volume
  (0x402), 8 Music Volume (0x403), 9 Engine Volume (0x404) at y 116 / 151 / 186 / 221, shown as "%s: < %d >"
  (0x8b800); left / right step the Sound Settings entry (0xc / 0xd / 0xb) 0..7 (the music volume applied at once,
  music_volume_mb) and play 0x17 when it moved.
- Restart, Track Select and Main Menu first ask (+0x610 = 2, +0x1cb0 = the item): "Are you sure?" (0x40c) at y 116,
  Yes (0x40a, item 0xb) 186, No (0x40b, 0xc) 221, the cursor on No (+0x60c = 0xc, 0x89c3c, its element +0x1aa4); No
  returns to the main page on that item; Yes: Restart 0x8b6f0
  (0x52abe8 = 1, the music restarts), Track Select 0x8b630 (0x5a1e7a = 1), Main Menu 0x8b5b0 (Time Attack with a new
  ghost goes to FE GHOSTSAVE first), each then a GAMELOGIC message.
- Draw (pausemenu_draw 0x89860 -> 0x7e5a0 -> 0x7e5e0 (0, 1.75)): the screen is cleared (so no HUD), then the
  echo texture (0x50e16c, the race frame) is drawn back full screen 5 times, offset by -o x 1.75 px for o = -2..2
  at alpha 255 x 0.5^n (255, 128, 64, 32, 16): three passes sideways, three up and down, the back buffer copied into
  the echo after each of the first five. On the console the blurred picture holds still (it does not smear on
  frame after frame), so which texture the copies read (draw_textured_poly 0xab420 binds through 0xcbb40, ESI not
  traced) and the blend are not settled. Then neon_draw_menu_bars
  (0xc2870: the front end's top / bottom bars and the logo). With 1 (the loader's multiplayer screen) the front end
  swirl (fe_draw_background 0xc2570) instead of the smear.
- Argon (ui\pause.cpp, the decisions in ui\pause_rules.cpp): Start, Esc or P pause while you drive; the race (physics,
  timers) and all sound hold (every playing voice paused) until resumed. Restart starts the race again from the grid,
  Track Select goes to FE COURSE SELECT (hidden in Tournament), Main Menu to FE MAIN; the volumes are saved with the
  profile at each step (argon writes at once; "Saving Race" keeps the game's second and its order). B steps back
  one page (Options / Are you sure? -> the main page on the item that opened it, with 0x18; nothing on the main page),
  argon addition. Replay Loaded / Session Ghost replay the ghost's lap (test `--ghost-replay <0|1> <seconds>` after
  a `--time-attack` pre-run). The sounds as above (RaceSfx::playOnListener, mixed while the race's voices hold). The
  controller message (App::watchControllers, menu::PadWatch, GameHud::drawReconnectMessage) for each racer whose pad
  was there once and goes (the keyboard alone never asks), in the race only (not on the result pages). The blur is
  made once on pausing from the 3D frame (the HUD is not drawn while paused) and held (drawPause); every item has its
  box, the selected one highlighted and filled (as the front end's rows). Test flag: `--pause 0..4` (0 main, 1
  Options, 2 Are you sure?, 3 "Saving Race" then Continue, 4 player 1's pad pulled out) with `--menu --quick-race <id>
  --race-go`. Tests: tests\frontend_tests.cpp (the sounds' rules, the second, the controller watch).
