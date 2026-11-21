# Front end (CFEShell and the main menu, from default.xbe)

Ported in `src\ui` (each page a class, as the game's: [argon/source.md](../argon/source.md); FE MAIN in
`ui\mainmenu.cpp`, the shell in `ui\menu.cpp`; argon opens on it; the Game Demos row, Debug here,
opens the editor). Argon code layout: "Argon code" below. Ghidra labels:
`tools\cmm\ghidra\label_frontend.py`.

| file | what |
|---|---|
| [cheats.md](cheats.md) | easter eggs and cheats |
| [game-modes.md](game-modes.md) | game modes and Time Attack |
| [intro-title.md](intro-title.md) | intro screens, title, attract demo |
| [loading.md](loading.md) | the loading screen |
| [main-menu.md](main-menu.md) | the main menu and argon's shell |
| [multiplayer.md](multiplayer.md) | multiplayer |
| [options.md](options.md) | options and custom music |
| [profiles.md](profiles.md) | profiles and the message box |
| [race-end.md](race-end.md) | race end and the pause menu |
| [race-setup.md](race-setup.md) | character, track and speed select |
| [tournament.md](tournament.md) | tournament and race scores |

## Shell (CFEShell, vtable 0x1a2890, object 0x597e90)
- Built by fe_shell_ctor 0x2f4b0, loaded by main_menu 0x30af0: models +0x4458 titlelogo, +0x3e48 dodechstart,
  +0x1ff8 options, +0x2608 multiplayer, +0x2c18 story, +0x3228 timeattack, +0x3838 quickrace, +0x4a68 earth,
  +0x5078 mars (frontend\mainmenu\... etc.), tourn_A/B/C, speedselect, profile, charbitmaps.
- Pages are named hud groups switched by fe_shell_goto_page 0x31390 ("FE MAIN", "FE OPTIONS", "FE CHARSELECT",
  "FE TITLE", "LOADER", ...).
- Cameras: +0x60c0 (scene) and +0x64c0, default fov 0.4 pi (0xada40), set by the shell constructor to pi x 0.2857
  (camera 1) and 20 deg (camera 2) (0x2f9a9 / 0x2f9c0); aspect 4:3, near 0.001, far 5000.
  0xae640 sets the camera **eye** (+0x36c); +0x378 is the point it looks at. Main menu (0x316e0 state 1): look at
  (0, 0, 1), eye (sin(t x s) x 0.02, cos(t x s) x 0.01, 0) with sway speed s = +0x9f4c = 0.
- Draw fe_shell_draw 0x2ffa0:
  1. Background fe_draw_background 0xc2570: frontend\background (loaded at app start, 0x76d60) with its own
     camera: eye (0, 0, -10) looking at the origin, fov pi x 0.2857, the model turned yaw sin(t / 2) x 15 deg,
     pitch sin(t / 4) x 15 deg. Then a 0x50000000 panel (-200, -5, 1040 x 490) and the feedback swirl 0xa7080
     (15, 8): the previous frame (texture 0x50e160) drawn back twice, zoomed by 1 + 15 / 320 and rotated +- 8 x t,
     then 0xa6eb0 copies the back buffer into that texture.
  2. Fade 0xc2810 (+0xa238): the feedback texture at that alpha.
  3. The scene (the page's item model).
  4. Menu bars neon_draw_menu_bars 0xc2870: panels (-400, -10) 1400 x 84 fill 0x304080c0 and (-400, 391)
     1400 x 84 fill 0xa0ffa030; data\qrlogo.dds at (62, 2) 256 x 128.
  5. Ticker: panel (62, 370) 506 x 20 fill 0x80303050, the description at (x, 372), x -= 2 a frame, back to 640
     once below -(width + 19); x resets to 640 when the description changes.
     Drawn only while the ticker element (shell +0x1230, global 0x5990c0) is visible: 0x304bc tests its flags
     (+0x1c4 = shell +0x13f4) bit 8 and skips panel and text together. Visibility is the element's virtual 0x4c
     (0xaf1c0: 1 clears bit 8, 0 sets it). A page shows it where it writes the description (fe string -> 0x598cb8,
     x = 640, SetVisible(1)), the slide-out hides it (0x316e0 modes 4 / 7 -> 0x31b84), so do the race start
     (0x23a17), FE AWARDS' leave (0x13341) and the Saving / Loading box (0x2b1a2, 0x2b3a3). A page that never writes
     one (FE AWARDS without an easter egg) has none. Argon: `FePage::ticker`, none by default (no panel,
     no text); an empty text keeps the panel.
- Title text +0x9ff0 (fe_shell_set_title 0x31510): fe string, right edge at 630 - 62, y 36, shrunk to 240 wide.
- Hints: g_fe_hdr_text_left / right at (62, 421) and right-aligned to 568 (y 421 = 0x195150), footers at y 445.
- Argon: `FeShell` (ui\fe_shell.hpp, `App::fe`) as this object: the pages, `goTo` (fe_shell_goto_page), the models
  (`FeModels`, ui\fe_models.hpp), the 60 Hz clock, title and ticker; update in ui\fe_shell.cpp, the draw above in
  ui\menu.cpp. Each page says what it puts into the render lists (`FePage::scene`, `FeScene` in ui\fe_scene.hpp: the
  models, camera 1 or 2, placement, near plane, light, the model fade), drawn by App::recordFrame (app_frame.cpp).
  Not ported: the background model frontend\background, the cameras' sway (speed 0 anyway), the home / return page
  (+0x1dc / +0x1631) and the idle clock of the attract demo (below).

## Page transitions (fe_shell_set_state 0x316e0)
- Hud element motion (0xad850, each frame): start +8, target +0x14, current +0x24 (x, y, z), +0x20 speed; +4 = 1
  paced: (target - start) / speed a frame (speed 10: 10 frames), +4 = 0: speed px a frame; both stop on the target.
  0xaf030 (from x, y, to x, y, speed) starts a paced move.
- States (shell +0xa228; the call also stamps +0xa22c with the clock): 0 hides the four hints (+0x5f8 / +0x804
  header left / right at y 421, +0xa10 / +0xc1c footer left / right at y 445); 1 (a page's enter, about 40 callers)
  shows the title and slides it in (0x31620: from y 36 - (its +0x3c) - 100 to 36, paced, x 630 - 62) and, unless the
  last state was 3 (never set), the hints in from their margin by their own width +0x3c (left from 62 - w to 62, right
  from 568 to 568 - w; paced); 4 / 7 (leaving a page, ~40 callers) slides the title back up by the same distance (paced)
  and the hints outward at 10 px a frame to 62 - w - 100 / 568 + 100, and hides +0x1230.
- FE MAIN, FE OPTIONS, FE CHOOSESAVE, FE CHARSELECT and FE COURSE SELECT call state 4, then their update opens the
  next page once the clock is 0.1 s (0x1a3508) past the pick (sequence 4 back, 7 forward), with state 0 just before;
  FE MAIN's rows (0x291e9) and FE COURSE SELECT's (0x1e920 -> 0x1e6a0) slide from 62 to 640 meanwhile (paced, so a
  little over half way when the page changes). The other pages call state 4 and switch at once, so only the next
  page's slide-in shows. Input is ignored in the sequences 4 / 7.
- Argon (ui\shell_slide.hpp `ShellSlide`, `FeShell::slideOut` / `goTo`, tests\menu_tests.cpp): every goTo slides
  the title and hints in; the five pages above call slideOut first, which holds their goTo for 0.1 s (input ignored)
  while the title and hints slide out and their rows leave. The hints slide with the shell only in the front end
  (`MenuKit::hintOutset`). Not checked: the title's +0x3c (taken as its drawn width; it may be its height), pages whose
  enter does not call state 1 (all slide in here). The switch's sound: "UI sounds" below.

## UI sounds (sound_play_ui 0xc3ca0)
Every call with an id in 0x10000..0x40000 (rawcalls.py c3ca0: 93 sites, ids 0..4 but the two noted below; [audio/README.md](../audio/README.md) "Front end"), plus the
shared list cursor 0xad070 (the +0x19c element's input: moving to a neighbour row, 0xad0e1 left / right, 0xad158 up /
down, plays 2; A calls the row's +0x5c) and the settings list (settings_input 0xac040 / enter 0xac6c0 / update
0xac8f0). "Press" plays as the button is read; "switch" plays in the page's update when the slide-out (sequence 4 / 7)
hands over to the next page. 0 SELECT, 1 BADSELECT, 2 ROLLOVER, 3 SCREENFORWARD, 4 SCREENBACK.

| Page (input / update) | Cursor (2) | A taken (0) | A refused (1) | B / Exit | A leaving | Switch |
|---|---|---|---|---|---|---|
| FE TITLE (0x34660) | - | - | - | - | - | forward 3 (0x3485e, 0x348a5) |
| FE CHOOSESAVE (0x1c070) | list, scroll 0x1c644 | - | - | 4 (0x1c807) | 3: a profile loads, New, Y (0x1c259, 0x1c6d4) | - |
| FE NAME ENTRY (0x38f10) | every d-pad press (0x39031, 0x3905a, 0x39083, 0x390ae) | A key, X delete, Y done (0x38fce, 0x38f64, 0x38f3d) | - | 4 (0x38f8d) | 3: done 0x3a110 with a valid name (0x3a150, 0x3a654) | - |
| FE MAIN (0x28e90 / 0x2a520) | up / down (0x29a51, 0x29b43) | - | Multiplayer, < 2 pads (0x29904) | - | - | back 4 (0x2a5e9), forward 3 (0x2a664) |
| FE CHARSELECT (0x163b0) | each move (0x16491, 0x16672, 0x16793), every player too | multiplayer join / pick (0x16f81; the pick that starts the race is silent) | locked; B undoing a player's pick (0x173e7) | 4 (0x173ff) | 3: A, Y Ship Data, X Character Data (0x16885, 0x17007, 0x1707e) | - |
| FE CHAR INFO (0x14a50), FE SHIP INFO (0x32e80), FE TRACK INFO (0x371e0) | scrolling silent | - | - | 4 (0x14a95, 0x32eb9, 0x3721d) | - | - |
| FE COURSE SELECT (0x1da10, 0x1e5e0, 0x1e920) | list | route choice opened (0x1e619 Mars, 0x1e66b) | locked (0x1e62b, 0x1e68f); B closing the route choice (0x1df57) | 4 (0x1dfa9) | 3: track / route (0x1e94a), Y Track Data (0x1dad7) | - |
| FE TOURNEYTYPE (0x35940) | list | - (New / Continue, the start: silent) | locked level (0x35af3) | 4 (0x359c6); B on New / Continue silent | - | - |
| FE SPEEDSELECT (0x33ac0) | up / down (0x33ef8, 0x33f88) | open speed (0x33b6e) | locked (0x33e75) | 4 (0x33afa) | - | - (0x33fa0 silent) |
| FE GHOSTSELECT (0x23b40 / 0x239c0) | list, scroll 0x240ce | A (0x23b96) | - | - | - | back 4 (0x23aaf), forward 3 (0x23a32) |
| FE FASTESTLAP (0x20a60 / 0x206c0), FE HIGHSCORES (0x25f20 / 0x25b20) | every left / right / up / down (0x20e90, 0x20efd, 0x20f7a; 0x2635c, 0x263e3, 0x26460) | A after a race (0x20e1f; 0x262c3) | - | **0** (0x20aaa; 0x25f84) | - | back 4 (0x20749, 0x207c5; 0x25ba9, 0x25c25) |
| FE GHOSTMANAGE (0x22000 / 0x21b30) | another track with ghosts, scroll (0x221f8, 0x22286, 0x2238e), list | - | - | B silent | - | back 4 (0x21b8d) |
| FE OPTIONS (0x2cef0 / 0x2d830) | list | - | - | 4 (B 0x2cf51; Exit 0x2dfd0 -> 0x2dfe8) | - | forward 3 (0x2d910) |
| FE OPTIONS MAIN / SOUND (0xac040 / 0xac8f0) | up / down; left / right that changed a value, not Engine Volume (0xac337) | Reset to Default answered Yes (0xac725) | - | - | - | back 4 (0xac98e), forward 3 Music Options (0xaca02) |
| FE MUSICSELECT (0x2bd80 / 0x2c430) | list; Default / Custom (0x2c19d) | - | Edit Custom Playlist on Default (0x2c120) | - | - | back 4 (0x2c504), forward 3 (0x2c48f) |
| FE LEVELMUSIC (0x27380 / 0x27650) | silent | silent | - | - | - | back 4 (0x27785) |
| FE EASTEREGGS (0x1f180) | list; switching silent | - | - | 4 (0x1f1a6, 0x1f1f9) | - | - |
| FE CHEATMENU (0x1a5e0) | list; switching silent | cheat activated (0x1a779) | locked, A on Infinite Shield / Turbo (0x1ab92, 0x1a7d1) | 4 (0x1a61d, Exit 0x1a86e, Enter Cheat Code 0x1a899) | - | - |
| FE OPTIONS CREDITS (0x2e110 / 0x2e390) | - | - | - | silent | - | back 4 (0x2e523) |
| FE AWARDS (0x133c0) | - | - | - | - | - | forward 3 (0x13427) |
| FE MESSAGEBOX (0x2b4b0) | every up / down on the buttons (0x2b841, 0x2b892) | A on an info box / a button (0x2b5f4) | - | - | - | - |

- No sound: FE INTROSCREENS, FE GHOSTSAVE (its message boxes), FE GOALDISPLAY, the controller message (kind 2 / 3).
  Not front-end input: 0x2eae2 (id 0xa, the Engine Volume test, 0x2ea60) and 0x325d9 (id 8, outside FE TITLE / FE
  INTROSCREENS); the loader's GOTOGAME (5) is app_loader.cpp's.
- Argon (ui\fe_sounds.hpp `PageSounds` per page, tests\frontend_tests.cpp): each page names its table
  (`FePage::sounds`) and reports events (`FePage::play`: Cursor, Pick, Refused, Cancel, Back, Forward, LeftBack,
  LeftForward); `FeShell::slideOut(LeftBack / LeftForward)` plays the switch sound when the held page opens. The pages
  that switch at once play both sounds together (the game's update runs the frame after). The list pages play 2 when
  their cursor moved (the game's lists link their rows; which ends wrap is the port's). Not compared by ear yet.

## Widescreen layout (g_widescreen 0x298ecc)
The dashboard's widescreen flag. The game still draws a 640 x 480 buffer, shown stretched to 16:9. 316 of the ~350
reads (e.g. 0x12095, 0x26bd0) place an item as (m + dx) x s + o: flag off m = 52 (the 4:3 safe margin), s = 1, o = 0;
flag on m = 0.75 x 52 - 80 = -41, s = 0.75, o = 80 (`g_ui_x_offset_16x9` 0x194fd8, 0.75 = 0x194fd4). Right-anchored
items use 640 - (m + dx) - width the same way, widths x s. The x 0.75 + 80 squeeze is undone by the 16:9 stretch, so
on screen the layout keeps its 4:3 proportions centred in the wide picture and only the margin changes: in the
centred 640 frame's units the safe margin moves from 52 to -41 (93 further out on each side; 66 units from a 16:9
screen's edge). Not yet sorted ([argon/todo.md](../argon/todo.md)): ~20 reads that scale widths only, and 0x4dc16 (80, 0.75), 0x36100 (40, 30),
0x4a741 (0.8), 0xcbc63 (45), 0x15332 / 0x1e6a2 (1280 / 640 wide). The 3D cameras: aspect / 0.75 (0xae550 / 0xae5c0).
In xemu (widescreen on) only some items follow the margin: the page chrome (title right, logo, menu rows, hints,
the ticker bar margin to margin), Character Data (info right, quote / biography left, the frame and the text wrap
margin to margin), Ship Data (name, portrait, panels left; stat bars right) and the results table (first column
from the left margin, the other column edges with the right one). Centred and unmoved: the High Scores / Track
Records tables, the character grid and Select Character's info block, the place / Congratulations lines.
Argon (ui\menu_common.hpp safeShift / atLeft / atRight / spanWidth / boxLeft, MenuKit::wrapWidth): the shift for the
window, 0 at 4:3, -93 at 16:9 and wider (between: the margin 52 from the window's edge); the pages above use it as
seen. Unchecked pages follow the nearer margin by position (left of x 300 / right of 340).

## Model fade (0xc2810)
The shell draw (0x30303) draws the shell's 3D model, then the feedback texture (0x50e160, the background echo)
again over the whole screen at alpha = shell +0xa238 (0x5a20c8) x 255, then the 2D. FE CHARSELECT (0x152df),
FE CHAR INFO (0x16372) and FE SHIP INFO (0x33519) set it to 1 - clamp(now - the time their model showed, 0, 1)
(+0x2b18, +0x1754, +0x1e0); load_frontend_characters sets 1; the main menu item draw sets 0 (0x13708). So a new
character or ship comes out of the background over one second. The other branch of 0x30303 is the locked
character's (shell +0xa23c, set to 1 by FE CHARSELECT's 0x162da when the character's bit is clear, 0 otherwise):
light 1 off, the model black, and the fade kept at at least 0.2 (0x1a358c), so the silhouette always has a fifth of
the echo over it; the normal branch draws the fade only when it is not 0. Argon: ui\menu.cpp (menu::modelFade),
from the model's load time on all three pages.

## Scene transition (0x2f2e0 / 0xa72a0)
0x2f2e0 (start, duration) copies the front buffer into the echo texture 0x50e160 and arms a fade (app +0xf61fe,
start +0xf6200, duration +0xf6204); while armed the echo is not refreshed, and 0xa72a0 draws the frozen frame
full screen at alpha 255 - 255 x clamp((now - start) / duration), turning itself off after. Started with 0.25 s on
leaving the front end for the loading screen (0x80556), on starting the race (0x97486) and on coming back to the
front end (0x30b86). 0xa7410 (called by the draw thread 0xb2aa0 after 0xa72a0) is a second timed effect of the same
shape: while +0xf6208 is set and (now - +0xf620c) / +0xf6210 is within 0..1 it draws a black 640 x 480 rectangle
(0xab290, no z, no culling), else it clears the flag. Nothing ever sets +0xf6208 (only 0x2f2ea and the init 0xa6da1
clear it; +0xf620c / +0xf6210 are never written): dead in this build, not ported. Argon
(ui\menu.cpp): every frame is copied after its render pass; a transition draws that copy fading out over 0.25 s,
over the 3D view and under the HUD / loading screen text (off in --shot runs).

## The hud cursor (vtable 0x19b048, ctor 0xacbb0)
The pages' cursor element (FE COURSE SELECT +0x1440, FE TOURNEYTYPE +0x1028, FE MESSAGEBOX +0x60c, ...) draws
nothing: its draw slots (+0x20 a stub, +0x2c = 0xacda0) only move its rectangle onto the selected element (the
element +/- the margins +0x1e0 / +0x1e4: 4 / 0 on the pages decoded), at once (mode +0x1d4 = 0, every page decoded),
at +0x1d8 px a frame (mode 1) or easing +0x1dc of the way (mode 2). Selecting (0xacc30 / 0xacce0) sets the element's
flag 0x10 (+0x1c4) and 0x2 (+0x1cc): the highlight is the selected element's own panel, which argon's rows draw.
Its input (0xad070) follows the elements' links (+0x1a8 / +0x1ac / +0x1b0 / +0x1b4) with UI sound 2 and A calls the
element (+0x5c). Nothing more to port.
