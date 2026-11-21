# Front end: tournament and race scores

## Race scores (SCOREMENU, POINTSMENU)
- The race end's order (every mode): POSTABLE -> A (0x8fa90) -> SCOREMENU (0x52afa8 = 3) -> A -> WINMENU / LOSEMENU
  (0x95650, after the tournament progress 0x95d80); X on SCOREMENU -> POINTSMENU (6), whose A also goes on to
  WINMENU / LOSEMENU (0x8d610).
- The numbers (profile block 0x1bab30; cashmodifiers.txt is read into +0x1978 .. +0x19e4 in file order: 1st..6th
  bonus, race_time, pickup, megaup, powerchip, weaponhit, kills, lap_time, air_time, the five speed modifiers, the nine
  run modifiers). Race points 0x84060 = race time x race_time_bonus + lap time x lap_time_bonus + weapon hits,
  power chips, mega-ups, pickups, kills x their bonuses (+ kills_bonus for the nemesis) + air time x air_time_bonus.
  Race score 0x84110 = (the place's bonus + the race points) x the speed modifier. 0x95930: the high score per
  character and track (Tournament 0x1bb108, Quick Race 0x1bb150, + (character x 0x2c + track) x 4); what the score
  beats it by (0 if not) is added to it and, in a tournament, to the character's cash (0x1bb0e8 + character x 0xb0).
- SCOREMENU (vtable 0x19c018: init 0x94390, update 0x94ff0, fill 0x95780): title "Race Scores" (race 0x28d); "Race
  Score:" (0x28f) y 100; "Points earned in race =" (0x290) y 140; "Race position bonus (<n> Place) =" (0x291, 0x410
  + place) y 160; "Race speed multiplier (<speed>)" (0x292, infotext 0xfa + speed) and "x %d" y 180; "Score this race
  =" (0x28b) y 220; "Previous High Score =" (0x28a) y 260; labels at 62, values right-aligned at 568; centred at (320,
  300) "NEW CASH = %d" (0x28c) / "No New Cash" (0x28e) in a tournament, "New High Score!" (0x521) / "No New High
  Score" (0x522) otherwise; "Earn new cash by beating your previous high score." (0x293) at y 340; hints "Press
  [BUTA] to continue." (0x519), "Show score breakdown - [BUTX]" (0x427).
- POINTSMENU (vtable 0x19c500: init 0x8bfc0, update 0x8d2c0): title "Points" (0x527); labels at 62, points
  right-aligned at 568: Power-ups (0x52a) y 100, Mega-ups (0x52b) 120, Bonus chips (0x52c) 140, Kills (0x52e) 160,
  Nemesis killed (0x52f) 180, Fast lap points (0x529) 210, Airtime (0x530) 230, Damage caused (0x52d) 250, Position
  bonus (0x531) 280, Speed modifier (0x532) 300, Total (0x533).
- Argon (app_score.cpp): the race points are argon's HUD score (its awards per kind fill the breakdown; the
  race-time part and Nemesis killed are not counted); the profile JSON keeps "cash", "hiTourney", "hiQuick"; High
  Scores (Options) shows them. Not checked: the fonts, Total's y (330 here), the run modifiers' use.
- UPGRADEMENU (around 0x9dfb0 .. 0x9ffff): per character three upgrades, Weapon Strength (0x3ff), Turbo/Lap (0x400),
  Shield (0x401), levels 0..7 (0x1bb0ec + character x 0xb0 + 0 / 1 / 2); buying a level (0x9fe40) costs table
  0x1bc518 [upgrade x 7 + level] from the character's cash; texts "Cash: " (0x4c3), "Level = " (0x4c5), "Upgrade
  Cost:" (0x4c6), "Maximum" (0x4c7), "Clear Changes" (0x4c8), "Bank Balance:" (0x51b). Prices: 0x82a70 (from
  0x81c40 at start-up, EAX = 0x1bab30) fills the table with immediates, three rows of 50000, 125000, 200000, 275000,
  350000, 425000, 500000 (= 50000 + 75000 x level, the same for all three); it is zero in the XBE file and every
  other reference only reads it. Found live in xemu with a write watchpoint (tools\xemu\watch.py).
  - Build 0x9dfb0: title "Upgrade" (0x3fe) right-aligned at (568, 36); "Bank Balance:%d" (0x51b) yellow centred at
    (320, 103); rows "Upgrade Weapon / Turbo / Shield" (0x44c..0x44e) at (62, 140 + 35 n) x 0.6, Continue (0x3ed) at
    (62, 270) and Clear Changes (0x4c8) at (62, 305) x 0.7.
  - Messages / update 0x9e7d0: "UPGRADEMENU ON" puts the cursor (+0x608) on Continue (3); messages 5 / 6 move it
    through 0..4 wrapping; 9 (A): rows 0..2 buy (upgrade_buy 0x9fe40: level < 7 and price <= cash, sound 0x18; else
    0x19), Continue leaves (a second of "Saving Race", the music dipping and back: [audio/music.md](../audio/music.md) "Music fades"; then 0x9ffc0 sends WINMENU / LOSEMENU again), Clear Changes
    puts back the levels and cash kept at 0x1bc87c.. (written by 0x620e0 at the race end). B is not handled.
  - Draw 0x9f040: pause_background (the race blurred); a panel per upgrade at x 296 (630 - 62 - 272), 272 wide, at
    y 140 (90 high) / 240 / 310 (60), fill 0x80404080, highlighted on its row and joined to it by a yellow line
    (0xaaca0); "Upgrade Cost: %d" yellow at (306, 142 / 242 / 312) x 0.8, or "Maximum" font 1 x 0.7; too dear: after
    the panel's details, an opaque panel filled 0xff404080 over them (0x9f771) and "Insufficient Funds" (0x516) font 0,
    white, wrapped at 262 and centred on it (0xb1230, flags 0x112), then the cost line on top; the row grey 0xff555555
    (else 0xff85dde5). Weapon: the icons (0x9f43e..0x9f68d, after the three panels, before the texts), every layer
    0xcc050 128 x 64 at (430, 150) (panel x + 272 - 138), u = level + 3: blend mode 1 (0xa9530), weaponbase (0x1be970)
    three times coloured 0xff000000 (darkening the panel under its alpha) and once 0xffffffff; u 10 (level 7): mode 1,
    weaponspecial (0x1be998) white, nothing else; else mode 2 (additive), white, the caps red 1 + (u > 5) + (u > 8),
    blue 1 + (u > 3) + (u > 6), yellow 1 + (u > 4) + (u > 7) (sprites from 0x597cd0, 0x28 apart: red1..3, blue1..3,
    yellow1..3), then the next one pulsing: u 3 blue2, 4 yellow2, 5 red2, 6 blue3, 7 yellow3, 8 red3 at the grey p x
    0x10101 - 0x1000000 (mode 2 still), u 9 (level 6) weaponspecial in mode 1 at (p << 24) + 0xffffff; p = 128 -
    fistp(sin(frame 0x439df0 x pi / 180 (0x5a211c) x 10) x -96.0 (0x1a3900)), so 32..224 with a 36-frame period.
    "Next upgrade:" (0x422) at (306, 198; 194 when the language 0x5293b0 is 2)
    and the next weapon's infotext (character x 10 + level + 3; seen in the game: Yuri level 0 "Double Venom L2") at
    (306, 210). Turbo /
    Shield: stat bar 0xc2420 at (306, 260 / 330) 252 x 22 filled (level + 3) x 10 %, the name right-aligned at 550,
    the value at 314 ("%.1fs" = 1.5 + 0.5 x level; shield 0x64030 = 20 x level + 100 / ((100 - armour) x 0.01),
    armour = weapon.nwp pilot block +4: Tara 104 .. Kutoo 140 at level 0), "Next upgrade: Turbo +0.5s" (0x423) at
    (306, 283), "... Shield +20" (0x424) at (306, 353).
  - Every row has its panel (build 0x9dfb0 sets element flag 2 on the five rows); the selected one highlighted and
    filled.
  - Argon (ui\upgrade.cpp): all of the above, with the menu bars and logo (0xc2870) as in the game's picture; the
    weapon icons' layers come from ui\upgrade_icons.cpp (tested in tests\frontend_tests.cpp), the frame counter is
    argon's clock x 60; the snapshot for Clear Changes is taken when the menu opens. Levels saved with the
    profile ("upgrades", 16 x 3). Test: `--menu --tourney-race 0 --char 0 --race-go --finish 1 --upgrade-menu --cash
    130000` (`--weapon-level n` sets the character's Weapon Strength first). Not compared with xemu: the icons' look.
- What the levels do in a race (race setup 0x3f3d0, the player only; multiplayer uses the best of the players):
  - Weapon Strength (+0): the pickup level caps (0x3f650; red / yellow / blue cap, special cap): 0: 1/1/1 0,
    1: 1/1/2 0, 2: 1/2/2 0, 3: 2/2/2 0, 4: 2/2/3 0, 5: 2/3/3 0, 6: 3/3/3 0, 7: 3/3/3 1. The AI crafts' caps come
    from the speed class and their +0x9f8 / +0x9f9 flags instead (same function).
  - Turbo (+1): each lap + 1.5 + 0.5 x level (0x3d5a0, [physics/turbo-flaps-cameras.md](../physics/turbo-flaps-cameras.md) "Turbo"); the craft's turbo scale +0x8e8 =
    (level + 3) x laps (0x58510; the craft update keeps +0xa38 = seconds / +0x8e8 x 3); a flag 0x439ff6 gives
    (1.5 + 0.5 x level) x laps at the start, in Time Attack only (0x429c0 is called by its update 0x43220 alone).
  - Shield (+2): 0x52cd24[craft]; used as level x 20 + 100 / ((100 - armour) x 0.01), armour = the craft's
    stats (+0x854) +0x90 +4: the shield size set up per craft, and in the HUD (0x4a100) the same minus 100 is the
    shield bar's upgrade term ([physics/hud.md](../physics/hud.md) "HUD"; how it enters the bar length is not checked).
  - "Fully upgrade all characters" (cheat 4, FE CHEATMENU 0x1a83b): all 16 characters' levels to 7 / 7 / 7.
  Argon: the weapon caps (app_pickups.cpp applyUpgradeCaps; the panel's "Caps from upgrade" can switch to a manual
  cap), the turbo per lap, the cheat; the shield size in a race (App::shieldSize: each craft starts with it, damage stops at
  0; the HUD bar shows the %, its length's upgrade term is not ported). The turbo scale +0x8e8 is not a HUD value:
  the fan reads the seconds (huddata +0xc); +0xa38 (seconds / +0x8e8 x 3) feeds the engine sound ([audio/README.md](../audio/README.md)).

## Tournament
- Flow: FE MAIN Tournament -> FE CHARSELECT (mode 0x52ccfc = 0) -> FE TOURNEYTYPE -> LOADER (0x35e50; B back to
  FE CHARSELECT). FE GOALDISPLAY is only reached from FE SPEEDSELECT in mode 0, which the tournament never opens.
- FE TOURNEYTYPE (vtable 0x19dfc0: build 0x35110, enter 0x36320, input 0x35940, update 0x35e50, ticker 0x365b0,
  start 0x35b20, cursor row 0x36530, show / hide 0x367a0; enter and input time out in Ghidra's decompiler). Title fe
  0xf0 "Tournament". Rows fe 0xf1..0xf5 (Novice, Amateur, Expert, Master, Redshift) at (62, 100 + 40 i), font 1 x
  0.75; levels above the highest open one (0x1bbc04, set with the speeds 0x1bb0e4) grey. "New Tournament" (0x140)
  and "Continue Tournament" (0x141) font 0 right-aligned at (568, 300 / 330), shown (+0x1218) when the level has
  races won. Ticker (0x365b0): a locked level fe 0x116 + level ("Win the Amateur tournament to unlock." ...);
  else "Progress: %d/%d races completed. Next race: <track>. <hint>" (fe 0x12c, the races won, the race count, fe
  0x12d, infotext 300 + track, and on the highest open level fe 0x104 + level; format 0x1a26a0 "%s%d/%d %s %s. %s").
  Start (0x35b20): 0x52cd08 = the level, the game speed = the level, the track = 0x79130(level, character).
- TOURNEYTYPE input (0x35940, read from the disassembly): with New / Continue shown, B hides them and puts the cursor
  back on the level; up / down move between them (the hud cursor's links, set in the build); A hides them, then on
  New Tournament opens FE MESSAGEBOX kind 5 (Yes / No) with fe 0x156 (+0x1219 remembers the question), on Continue
  starts the race. Without them: B plays UI sound 4 and leaves (0.1 s later FE CHARSELECT, 0x35e50); A on a level
  above 0x1bbc04 plays UI sound 1; A with no race won (progress 0 or -1) sets the progress to 0 and starts; else New
  / Continue come up with the cursor on Continue. Enter (0x36320, also the message handler for 0xfe00 / 0xfe01):
  coming back from the question, Yes (+0x1f4 == 1.0) clears the level's progress and starts; otherwise (and on a
  normal enter) the cursor goes to 0x1bbbec, the level of the last tournament started (the update stores +0x1214
  there when it leaves for the LOADER; it is part of the profile), New / Continue hidden, the rows coloured 0xff85dde5
  (open) / 0xff555555, hints 0x32 / 0x33.
- TOURNEYTYPE models (update 0x35e50, while +0x121a): the speedometer shell model (below, "Select Speed") with its
  needle on the cursor's level; then per level i, from x = 52 + 10 + the row's width (+0x3c) + 40: the trophy
  data\frontend\tourn_C.dds (shell +0x1fd0, loaded by main_menu 0x30e86) 40 x 40 at y 90 + 40 i when held (0x1bb0ef +
  character x 0xb0 + i), then x + 40; while the level's progress is above 0, one icon per race 30 x 30 at y 95 + 40 i,
  30 apart: tourn_B (+0x1fa8) for the races won, tourn_A (+0x1f80) for the rest (0xcc5e0, white, uv 0..1).
- Races per level (0xa1320): Novice 2, Amateur 4, Expert 4, Master 5, Redshift 9. Progress 0x1bb0f4 + (character x
  44 + level) x 4: races won so far (-1 = done). Only a win moves it on (0x95dc7: the player first in the finish
  order); after the last race the tournament is won (0x84160) and the progress is -1.
- The track of race n (0x79130; b = character & 7, nemeses 8..15 race the other routes of their home tracks): race 0
  the home track (= the character id; Redshift: its other route); 0x791f0 sends Amateur / Expert race 3 and Master
  race 4 to the home track's other route (the nemesis challenge: the player and the nemesis alone, [physics/README.md](../physics/README.md)
  "Grid"); Master race 3 and Redshift race 7 are Draco (7) unless Draco is the home;
  Redshift race 8 the home track again; otherwise the table 0x1b9438 (8 rows of 8, per b) at b x 8 + offset + n,
  offset Expert 2, Master 4, else 0. Character 0 (Tara): Novice 0 4, Amateur 0 4 3 8, Expert 0 5 1 8, Master 0 2 6 7
  8, Redshift 8 4 3 5 1 2 6 7 0 (`--tourney-order` prints all).
- The tournament win (profile_tournament_won 0x84160): the trophy (0x1bb0ef + character x 0xb0 + level); above
  Novice an original character unlocks its nemesis (+8; not Draco's owner 7) and its home track's other route; a
  nemesis winning Expert or Master for the first of the two unlocks the next easter egg. Amateur opens Expert (2),
  Expert opens Master (3); Master opens Draco (track 7), Draco's owner (character 7) once the seven originals hold
  Master, Draco's nemesis (15) when 7 wins it, and Redshift (4) once every open character 0..15 holds Master (the
  loop 0x8436f runs to 0x1bbb42 = 0x1bb0f2 + 15 x 0xb0); Redshift opens Mars once every open character holds Redshift.
  The win first clears the awards record 0x1bc890 (game state 0x1bab30 + 0x1d60) and fills it with what is new:
  +0x1d60 the level if its trophy is new, +0x1d64 the nemesis, +0x1d68 the other route, +0x1d6c the level opened,
  +0x1d70 Ka Hekili, +0x1d71 Kutoo, +0x1d72 Draco, +0x1d74 the easter egg, +0x1d78 Mars (-1 / 0: nothing).
- FE AWARDS (build 0x128d0, enter 0x12e70, input 0x12b60, update 0x133c0, frame 0x13790, rows 0x137f0): WINMENU after
  the last race sets 0x52ac00 (0xa1bf9); FE MAIN (0x29e60) then goes to FE AWARDS, which clears it on leaving. Title
  "Rewards" (fe 0x1c3, 0x12ec8); "Congratulations!" (0x1b8) right-aligned at (568, 100); a gradient frame (52, 195,
  536 x 170); row 0 at (62, 200) "You have won the <level> Championship trophy!" (0x1b9 + level), then rows from
  y 220 + 20 n: Ka Hekili (0x1c2), Kutoo (0x1c0), "You now have access to " (0x1c4) + infotext 200 + nemesis, "You
  unlocked " (0x1be) + infotext 300 + track, the level opened (0x1c6..0x1c8), Draco Island (0x1bf), Mars (0x1c1),
  the easter egg (0x1c9 + n, and the ticker 0x1d1 from x 640, 0x141cf: the only place the page shows the ticker, so
  without an egg no ticker panel or text; `awardsShowTicker`); "Press [BUTA] to continue" (0x3c). No new trophy:
  the page leaves at once. Models, render list +0x690c (camera 2): the first unlocked character of Ka Hekili / Kutoo /
  the nemesis at its 0x1b9a88 position, animated from the enter; the level's objects\fe_trophy1..5 at (50, 70, -550).
  A or START: UI sound 3, FE MAIN.
  Argon (ui\awards.cpp, AwardsPage): as above; `--menu-page 26 --char <c> --speed <level>` (with `--new-profile`
  for the unlocks). The trophy (0x13742: rotation Y 0 (0xf2f3e), at (50, 70, -550), camera 2) sits right of centre
  over its glowing backdrop discs; its revol2..4 bones only share the ships' weapon-part name, so the trophies load
  with `ModelUse::weaponParts` off (else the cup, the stand and the discs are hidden). To compare with the game: the
  rows' font (drawn in font 0).
- Post-race (WINMENU 0xa1bd0, LOSEMENU 0x85a50): WINMENU Upgrade Menu (0x528), Next Race (0x3f3), View Replay, Save and
  Quit (0x40f; Quit 0x41e while 0x459a8c is set: the game runs without a profile to save, set by "Continue without
  saving." and cleared when FE CHOOSESAVE loads one, 0x1c1c1); after the last race Upgrade Menu, View Replay, Continue (0x3ed).
  LOSEMENU Upgrade Menu, Race Again, View Replay, Main Menu.
- Argon (ui\tourney.cpp, the tournament app_tourney.cpp): page 22; the profile JSON keeps "tourney" (the 80 progress values) and "trophies" (80
  bits) besides the unlocks. Next Race loads the next track through the loading screen; Save and Quit / Continue /
  Main Menu go to FE MAIN (after the last race through FE AWARDS). FE TOURNEYTYPE's input, the speedometer and the
  icons are ported as above (2026-11-14; ui\tourney_icons.cpp); "Save and Quit" reads "Quit" with no profile loaded.
  The profile JSON keeps "tourneyLevel" (0x1bbbec). The tournament loading-screen hints are ported (LOADER). Test flags:
  `--menu-page 22` (with `--char`, `--speed <level>`, `--tourney-done <races won>`, `--trophy <level>` repeatable),
  `--tourney-race <level>` (with `--char`, `--tourney-done`, `--race-go`), `--tourney-order`.
