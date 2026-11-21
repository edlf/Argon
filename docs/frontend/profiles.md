# Front end: profiles and the message box

## Profiles: "FE CHOOSESAVE" (vtable 0x19e090) and "FE NAME ENTRY" (vtable 0x1a2478)
FE TITLE leaves to FE CHOOSESAVE; a profile chosen or created goes on to FE MAIN.
- Profile (0x1baf74, reset by 0x83b60): best laps +0 (90 floats = 999999), +0x168 = 1, +0x16c characters 0x7f (the
  seven originals), +0x170 = 1, +0xc74 game speed (0x1bbbe8), +0xc88 tracks 0x7f (0..6: Draco, the second routes and
  Mars locked), +0xc8c Mars 0, +0xc90 = 1 (speed classes), the rest cleared (trophies +0xc94.., flags +0xcb3 = 1).
  0x83c40 (called before every profile save and after a load) copies the profile's best laps that beat the global lap
  records (0x1bbc38) into those records, with the profile name 0x459c98; it does not fill the profile from defaults.
  The game saves the profile as an Xbox save (U:\): savegame.sav + SaveMeta.xbx, every field mapped in [saves/README.md](../saves/README.md)
  (with tools\saves\profiles.py to read, edit, sign and convert them).
- FE CHOOSESAVE: build 0x1b440, enter 0x1c850, update 0x1bab0, input 0x1c070, draw 0x1be00, list 0x1cb30. Title
  fe 0x64 "Select Profile"; ticker fe 0x6a; hints Back / Select, [BUTY] Create New Profile (0x34) / Delete Profile
  [BUTX] (0x35). Draw: gradient frames (52, 160) 536 x 20 (swapped colours) and (52, 180) 536 x 180, and a rotated
  quad at (320, 260) 640 wide behind them (not ported). Header fe 0x68 "Select Profile" at (112, 160) and fe 0x6c
  "Percentage Complete" right at 568, orange (0xffffbe64). Five rows: name at (112, 210 + 25 i), "%d%%" right at
  568; the selected row x 1.5 at y 205 + 25 i; an unreadable save "Invalid profile" (0x69). The list (0xbfb60
  with its fourth flag, as enter calls it) always starts with an entry named fe 0x6d "Create New Profile" (no
  percentage, list 0x1cb30), the saves after it; the cursor starts on the first save when there is one (0x1cef0).
  So the count is never 0 there and the no-saves case (fe 0x65 "Press [BUTA] to create profile." centred at
  (320, 240), the hints hidden) does not show. Arrows icons\up / down at (112, 181 / 335), 40 x 20.
  Input: up / down (scrolls past 5), A on "Create New Profile" or Y -> FE NAME ENTRY, A on a save loads it (fails:
  0x1f4), X on a save asks 0x67 "Are you sure you want to delete the profile: " + name, B -> FE TITLE.
- FE NAME ENTRY: build 0x37c70 / keys 0x37d90 / layout 0x389d0 / case 0x38c70, enter 0x39630, update 0x39480,
  input 0x38f10, draw 0x3ab30, key press 0x39ca0, Delete 0x3aac0, Done 0x3a110. Title 0x2d0 "Enter Name" (0x304
  for cheats); ticker 0x2dc; hints Back / Select, Backspace - [BUTX] (0x3e) / [BUTY] - Done (0x3f). Frames
  (173, 150) 420 x 190 keys, (50, 150) 123 x 190 special keys, (50, 100) 543 x 45 name (swapped). The name at
  (590, 105), orange: "PLAYER" (0x4b0) + the first free number, replaced by the first key, at most 9 characters,
  "_" after it. Keys (2 pages x 5 rows, code / slot width; x from 70, + slot + 6 (+ 4 after the first), rows
  y 160 + 36 r; panel sizes 16 x 20, specials 85, Space 140, Done 380): [Delete] 1..0 / [CAPS] A..J / [Accents]
  K..T / U..Z Space / Done; accents: [Delete] Ã Å Æ Ø / [CAPS] À..Ì / [Alphabet] Í..ß / Ù..Ÿ Space / Done. Labels
  0x2d1 Space, 0x2d2 Delete, 0x2d4 CAPS, 0x2d5 Alphabet, 0x2d7 Accents, 0x2d8 Done; letters lower case unless
  CAPS. The cursor starts on Done. A presses, X deletes, Y = Done, B -> FE CHOOSESAVE. Done: empty -> 0x2da, only
  spaces -> 0x2dd, taken -> 0x2db, else the profile reset is saved (a name starting "CHEAT" (0x4b1) also runs
  0x83af0) -> FE MAIN.
- Argon (saves\profile_store.cpp, app\front_end_stores.cpp ProfileSwitcher; the pages ui\choosesave.cpp, ui\keyboard.cpp): pages 7 / 8; one JSON file per profile in <game>\profiles (name, characters, tracks,
  mars, speeds, percent); the chosen profile's characters / tracks / Mars drive the character and track pages
  (`--unlock-all` ignores them; starting without a profile, e.g. `--no-title`, keeps everything unlocked). The PC
  keyboard can type the name. FE CHOOSESAVE's background (fe_choosesave_draw 0x1be00): the ID card
  data\frontend\profile\tex0.dds (shell +0x585c, loaded at 0x30ef9) through hud_draw_rotated_quad (320, 260, -20 deg
  + 180 (the 0x5a211c factor is pi / 180), -160, 160, 640, white), additive ONE / ONE, clipped by a viewport (52, 160)
  536 x 200, under the two frames (ui\picture_quad.cpp; argon had the card mirrored left to right before
  2026-11-14).
- FE NAME ENTRY's moves (input 0x38f10, one a frame: down 0x39230, up 0x390d0, left 0x39360, right 0x393f0): the
  cursor is a row (+0x14a28, 0..4) and column (+0x14a2c, of 14 per row) with a remembered column (+0x14a30). Down:
  from row 2 past column 6 onto Space (column 7), remembering the column; from row 3 onto Done (column 1),
  remembering the letter's column (not Space's); from Done round to row 0 at the remembered column, at most 10. Up:
  from row 0 past column 1 round onto Done, remembering; from Space to the remembered column when it was past 6;
  from Done to the remembered column, at most 7. Left / right go round the row's key count (0xa5ec). Every move goes
  on while it lands on no key (code 0), and in the game's languages 1 / 2 (German / French; 0x12807: 0 Japanese,
  3 English, 4 Italian) on the Accents key (0x10004). Argon: ui\keyboard_moves.cpp.
- Percentage Complete: the save list (0xbfb60, entries of 0x214 bytes: +0 invalid, +4 percent, +8 name, +0x210 next)
  reads each profile and runs 0xc03a0 on it: the sum of 0x82910 per character (the highest trophy level held + 1,
  0 for none: 0..5 x 16), +1 per unlocked character 8..15 (bits 8..15 of 0x1bb0e0), +1 per unlocked easter egg
  (0x1bbc08..0f), +1 for 0x1bbc34, +1 for 0x1bbc35, +2 for 0x1bbc36: 100 in all. The three flags are set by POSTABLE
  (0x8ecc7) after a Quick Race (mode 1) the player won (first in the finish order, not the attract demo 0x52abe0) at
  Redshift speed (0x52cd04 = 4): on Mars' Martian route (track 0x10) 0x1bbc34, on its Terraformed route (0x11)
  0x1bbc35, with easter egg 0 switched on (0x1bbc10) 0x1bbc36. Argon (logic\profile_progress.cpp,
  logic\progress_keeper.cpp): worked out from each profile file on load and save; the JSON keeps "redshiftWins" (bits 1 / 2 /
  4) and "tourneyLevel" (0x1bbbec) beside the tournament progress, trophies, cash, upgrades, scores and unlocks that
  were saved before.

## FE MESSAGEBOX (vtable 0x19da68)
- init 0x2a9c0, open 0x2b000 (text, kind, seconds), update / draw 0x2ad70, input 0x2b4b0, close 0x2b470. A shell
  page of its own: the caller's page is left (fe_shell_goto_page), the shell background, bars, title and hint bars
  stay. The text: white (font_draw_text colour 0xffffffff), font 0, from (62, 180), 516 wide, up to 10 lines
  (kinds 2 / 3, the controller messages, flash grey 255 -> 64). Buttons font 0 centred on 320 at y 280 / 300 / 320
  with the hud cursor (+0x60c).
- Kinds: 0 one button "Press [BUTA] to continue" (fe 0x3c; profile errors, 0x1c157 / 0x3a497 ...; A only, no B); 1 timed, no
  button, closes once the clock passes open + seconds (no caller found); 2 / 3 "Please reconnect the controller and
  press START to continue." (fe 7), grey flashing 255 -> 64 by 16 a frame, closed by A / START (+0x88 & 0x110) on any pad
  (kind 3, multiplayer: the caller's text, only the pad +0x1f4 closes it); 4 (profile: leaving FE OPTIONS 0x2d425,
  FE FASTESTLAP's save 0x20dfd, 0x25fb3) / 0xb (ghost, FE GHOSTSAVE 0x22ff2 / 0x2307a) "Saving" (0x200), 0xa (ghost, FE
  GHOSTSELECT 0x23faa) "Loading" (0x201): the text empty, the label in the left header hint (right hint, footers,
  title 0x5a1e80 and 0x5990c0 hidden), no frame; once the clock passes open + 0.5 s (0x1a3504) the box saves / loads
  in its input 0x2b4b0 and closes (kind 4 failing: kind 0 with fe 0x1f8 "This is an invalid save." / 0x2d9 / 0x1f9;
  kind 0xa failing: 0x1f8); 5 / 0xd Yes (fe 3) / No (fe 2) (delete profile 0x1c7ec, Reset to Default 0xac126, ...);
  6 "Yes, replace the old ghost" (0x1a7) / "No, keep the old ghost" (0x1a8); 7 / 8 / 9 three buttons "Continue
  without saving." (0x244), "Return to Select Profile" (0x246, kind 8) / "Return to Select Speed" (0x247), "Launch
  the Xbox Dashboard." (0x245), the answer in +0x1f4 (1 / 0 / 2; kind 7 takes no input): the no-free-blocks cases
  (FE NAME ENTRY 0x3a74a kind 8 with 0x2d9, ghost save 0x24838 kind 9); 0xe "Delete and create a new one" (0x1ff)
  under "Your fastest lap data is invalid." (0x1fe, 0x34e92).
- Open 0x2b000 (jump table 0x2b438 / 0x2b45c): kind 1 sets +0x1f4 = now + seconds; kinds 7 / 9 show 0x247 as the
  second button, kind 8 0x246; all three show the third. The hud cursor starts on the first button, for kinds 5..9
  (and 0xd) on the second (0x2b408): No, "No, keep the old ghost", Return to ...
- Input 0x2b4b0 (jump table 0x2b8ec / 0x2b908): kinds 0 / 0xe close on A (UI sound 0); kind 1 closes once the clock
  passes +0x1f4 (0x2b60b); kinds 5, 6, 8, 9 (0x2b7cc): A answers by the cursor's button (+0x1f4 = 1.0 / 0 / 2.0 for
  buttons 0 / 1 / 2), up / down (UI sound 2) go round the buttons (two swap; three: up 2 -> 1 -> 0 -> 2, down the
  other way), B does nothing; kinds 7, 0xc, 0xd take no input.
- Frame (vtable +0x2c, 0x2ace0): the gradient frame (52, 150) 536 x 200 (swapped) behind every kind but 4 / 0xa / 0xb.
- No pad at all (0x30aa0): the shell opens kind 2 on every page but FE MESSAGEBOX / FE TITLE / FE INTROSCREENS
  (fe_shell_update 0x306b0); a player's pad removed (0x30a70): kind 2 with fe 0x44c + pad (kind 3 in multiplayer).
  Inside the box (+0x3fc) the element +0x400 flashes and the buttons hide until a press.
- Argon (ui\messagebox.cpp, `MessageBoxPage`, `App::fe->messageBox()`): kinds 0, 2, 4 / 0xb, 0xa, 5 and 6
  (`Kind::Info`, `Controller`, `Saving`, `Loading`, `YesNo`, `ReplaceGhost`), 1 (`Timed`, `openTimed`), 7, 8, 9
  (`NoFreeBlocks`, `NoFreeBlocksProfile`, `NoFreeBlocksGhost`; no caller on a PC)
  with the frame (ui\message_buttons.cpp: the buttons, the cursor's start and moves). Saving / Loading answer
  yes after 0.5 s and the caller saves / loads then: FE GHOSTSAVE's ghost, FE GHOSTSELECT's ghost (failing: fe 0x1f8),
  leaving FE OPTIONS after a profile was written there (argon writes each change at once). The controller
  message opens when the last gamepad is unplugged (the keyboard alone never asks), not on the title, intro or loading
  screens. `--message 1..8` shows info, yes / no, controller, Saving, timed (3 s), kinds 8, 9, 7. Not ported: kind 3
  (multiplayer), 0xe (the lap records' file is JSON here), the per-pad message 0x44c + pad, the +0x3fc variant. The shell owns it beside the pages; while it is open `FeShell::active` hands it the input and the
  draw (the current page keeps the title and ticker), and `FeShell::goTo` closes it (fe_shell_goto_page). The page
  that asks passes itself as a `MessageListener` and gets `messageClosed(yes)` once the box has closed. Not drawn:
  the caller's hint bars.
