# Front end: the main menu and argon's shell

## Main menu page "FE MAIN" (vtable 0x19d798)
- Build fe_main_build 0x288b0: rows = fe strings 0x79 Tournament, 0x7a Time Attack, 0x7b Quick Race,
  0x7c Multiplayer, 0x7d Options, 0x7e Game Demos (only when d:\xdemos exists, else hidden) at x 52 + 10,
  y 105 + 40 i, scale 0.75, flag 2 (a panel behind the text). Cursor element +0xe1c (hud_cursor_select 0xacce0):
  marks the selected row (+0x1c4 |= 0x10) and sizes itself to it, margin 4.
- Text element draw 0xad620 with flag 2: panel at (x - h / 2, y) (w + h) x h; normal rows outline only
  (no highlight, glow), the selected row highlighted and filled 0xffa08040. Default text colour 0xff85dde5.
- Enter fe_main_enter 0x29d30: hints 0x32 "Select - [BUTA]" / 0x33 "[BUTB] - Back", title 0x78 "Main Menu",
  description and item model of the selection; rows start at x 640 and move to 62 by (62 - 640) / 10 a frame.
- Input fe_main_input 0x28e90: up / down move the selection +0x102c (wrapping 0..4, or 0..5 with demos);
  descriptions 0x8c..0x91. A / START: 0 tournament, 1 time attack, 2 quick race (FE CHARSELECT, modes 0 / 3 / 1),
  3 multiplayer (needs 2 pads), 4 FE OPTIONS, 5 demos (message box 0x4, then XDemos.xbe 0x2a860). B: back to the
  title. Leaving moves the rows back to x 640; the next page starts 0.1 s later (fe_main_update 0x2a520).
- Item model fe_main_draw_item 0x2a720 (models frontend\mainmenu\story / timeattack / quickrace / multiplayer /
  options; none for demos): rotation 0, translation (-4, -2 for quick race else -1, 0), multiplayer's x / y axes
  x 0.75, drawn with the shell camera. The meshes sit 17..21 m in front of it.
- Button tags in text (0x76d60): [BUTA] icons\a.dds, B, X, Y, L (left), R (right), U (up), D (down) 32 x 32,
  "...FIX" the same, [BUTBSMALL] 16 x 16.

## Argon port (ui\menu.cpp, ui\mainmenu.cpp)
- The app opens on the menu unless --track / --drive / --shot is given (--menu / --editor force it;
  --menu-sel n picks the row). The editor panel has a "Main menu" button.
- Keys: up / down, Enter / Space; pad d-pad / left stick, A / START.
- Quick Race starts a race on the editor's current track with you driving; Game Demos (shown as Debug)
  opens the editor; argon's Argon Options row opens its settings page; its last row, Exit, asks, then saves the profile and closes. The other rows (no screen
  yet) are dimmed and do nothing.
- The game's 640 x 480 screen is centred; the bars stretch to the window width; cameras keep the vertical fov.
- Background: 0xc2570 draws **no model** (frontend\background, with its scrolling binary-digit layer, is not
  shown on this page). It clears to black, draws the darkening panel, then the two feedback layers, and copies the
  result into the feedback texture (0xa6eb0, from the surface GetBackBuffer2(0) saved at start-up); everything
  else (scene, bars, text) is drawn after the copy. The background is therefore the panel's plasma glow smeared
  outward by the zoom / rotation. (Copying the whole finished frame instead echoes all the text: wrong.)
- Argon: on each 60 Hz tick the ImGui draw of the menu clears, draws the darkening panel and the feedback layers,
  ends the render pass (menuCutCallback), copies the swapchain image into the feedback texture, starts the pass
  again and draws that copy first; frames between ticks only draw the last copy, so the frame rate stays unlocked.
  Below 60 fps it steps once per drawn frame (slower than the game). Row slides use the same clock; the ticker
  moves by the frame time instead (the game's 2 px per 60 Hz frame = 120 px a second, smooth at any refresh rate).
- Panel fills (0xc1b40 at 0xc1fb0): corners 0..2 = the colour argument (EBX), corners 3..5 = the caller's EAX.
  Darkening: the wrapper 0xc1b00 passes 0x50000000 in both (uniform). Bars (0xc2870): top fill 0x304080c0 with
  EAX 0xa0ffa030, bottom fill 0xa0ffa030 with EAX 0x304080c0, so each bar is strong orange on the edge facing the
  menu and faint blue on the outer edge. Selected row 0xffa08040 in both, ticker 0x80303050 in both.
- Selected row blink (0xad620): text RGB x (sin((vblank count % 360) x 10 deg) x 0.25 + 0.75), clamped, not for the
  grey 0xff555555: 0.5 .. 1.0 brightness, one cycle every 36 vblanks (0.6 s).
- Hints: the left header text gets 0x33 "[BUTB] - Back", the right one 0x32 "Select - [BUTA]" (as in the game).
- Text sizes, measured on xemu screenshots (widescreen on and off: the same size in the 640 x 480 layout): the
  scales the code sets: title font 1 x 1, rows font 1 x 0.75 (rows set font 1 in 0x288b0: +0x1d4 = 1), description
  font 0 x 1, hints x 1 with their icons (drawn at 397 rather than the code's 421). An earlier reading of 0.75 of
  these (a 640 x 480 screenshot) made argon's front-end text 1 / 0.75 small. Row panels are the font 1 line x 0.75 = 26 high (x - 13 .. text + 13),
  the text centred in them.
- Multiplayer is grey (0xff555555, 0x2a520) with fewer than two pads connected.
- Panels take the game's last argument (plasma uv scale: 0.25 darkening, 0.4 bars). Their plasma scrolls by the
  vblank count (0xc16f0 reads global_frame_counter_2, bumped 60 times a second by 0xb2670), one loop every
  1000 vblanks; it is not scaled, so on the big panels it drifts slowly across the screen.
- Frame rate: the main loop (0xb2aa0) waits for +0x10 of 0x39d578 vblanks per frame (0x39d588): 1 in races and
  the front end, 2 while a movie / cutscene plays (CMoviePlayer::Open 0xe03e0, 0x9a810), back to 1 when it closes
  (0x323a0). So the front end runs at 60 Hz.
- Scene lights (for the item models; 0x2ffa0 -> 0xa80d0 / 0xa7f60, 0xa9be0): light 0 colour 0xcccccc along
  (-0.5, -1, -1), light 1 off (0xa7ea0 colour 0), ambient 0x707070 (+0x605c, written by 0xa83c0 the frame
  before). Lighting is on for the scene (0xa9be0 sets 0x50e324 / 0x1188b8 = 1); the only material (0xa6a60,
  SetMaterial at init) is diffuse / ambient / specular 1, emissive 0, power 4. Argon lights the menu's prelit
  meshes too (scene toggles2.y): vertex colour x (ambient + light x N.L).

## Argon start-up
Argon opens on the title screen (then FE CHOOSESAVE / FE NAME ENTRY, then FE MAIN) with no track loaded; the editor and the race only run once one is picked:
Quick Race (character, then track) shows the loading screen, loads that track and starts the race on A, Game Demos opens the editor on its last
track (or `--track` / bracken). The editor's "Main menu" button (F1 shows the panel) unloads the track again.
`--editor`, `--track`, `--drive` and `--shot` without `--menu` start in the editor as before. Test flags:
`--menu-page 4 --course <row>`, `--quick-race <track id>` (with `--char <id>`; add `--race-go` to skip the
loading screen's A), `--menu-page 0..22` (the `menu::Page` numbers: 9 Options ... 17 Easter Eggs, 18 Cheats, 19 Edit
Playlist, 20 Select Speed, 21 Track Data with `--course <row>`, 22 Tournament), `--settings "8=7;15=1;"` (the settings as a profile's "options", decimal types: Brightness 7, HUD Map Off), `--egg <0..7>` (that easter egg on in the race, the FE EASTEREGGS row order; repeatable).

## Argon code
- Pages: `menu::Page` (ui\page_id.hpp) names them (PageMain 0 ... PageTourneyType 22, the `--menu-page` numbers);
  `FeShell` (ui\fe_shell.hpp) keeps one `FePage` object per id. Each page overrides `input`, `draw`, `step`, `title`
  (its shell title, fe id), `ticker` and `bare` (the loading screen, title and credits: only their own elements over
  the background). FE MAIN is `MainMenuPage` (ui\mainmenu.cpp).
- Shared pieces (`MenuKit`, ui\menu_kit.cpp; ui\menu_common.hpp): `drawMenuRow` (the text element 0xad620 with flag 2: the panel
  at x - h / 2, w + h, filled and highlighted while selected, the text centred and blinking; left, right or centre
  aligned; `idlePanel` false for the choices that only show a panel under the cursor), `drawHints` (the hint texts
  at 62 / 568, y 397, x 1), `menuCursor` / `stepCursor` (up / down wrapping; Game / Sound Settings run up = next),
  `slideRows` (rows from x 640 to 62 in 10 steps, and back out). Colours (`kText` 0xff85dde5, `kGrey` 0xff555555,
  `kOrange` 0xffffbe64) and the text factor: `menu::fe::kFe` 1 for the front end (was 0.75), `menu::race::kFe` 1 for the
  race's menus (PAUSEMENU, UPGRADEMENU, the result pages).
- Rows that differ from the game's element keep their own code: Name Entry's fixed-width keys, Course Select's track
  rows (filled while the route choice is open, no blink when locked), the playlist's half-margin cursor, FE MAIN's
  rows (grey Multiplayer does not blink).
- Tests: `test.bat` (tests\menu_tests.cpp) checks the helpers' arithmetic against the pages'
  original inline code.
