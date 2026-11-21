# Front end: the loading screen

## Loading screen "LOADER" (0x808e0)
Set up in neon_app_init_load (0x77c49) with an event "LOADERTHREAD"; the loader thread draws a frame (0x808e0)
while the track loads (script sequence seq_loadtrack -> seq_loadingtrack -> seq_loadtrackcomplete, 0xa5530).
Strings: race_strings.str (table 0x52c8a4), infotext (0x52c8b0). Single player:
- Background: 0xa72a0 = the scene transition (below) fading the last front-end frame out; underneath, the
  cleared screen.
- Title box: "%s\n[F:1]%s\n[F:0]%s" = race 0x51a "Welcome to" / the track (infotext 300 + id) / race 0x550 + id
  ("Home circuit of Tara Vivas", 0x560 / 0x561 Martian / Terraformed route), font 0 at 1.0, at (62 + 8, 36 + 8);
  panel (62, 36) of the text + (120, 16), fill 0x80401000, scale 0.5; for id < 16 the pilot icon id (track i
  is pilot i's home circuit) 96 x 96 at (62 + text w + 16, 44).
- Hint panel (62, 170) 516 x 180 (no fill, scale 0.5): race 0x420 "Hint:" (72, 172) 0.65; the hint race 0x618 +
  0x1bbc2c (Quick Race; Tournament 0x5dc +, Time Attack 0x64a +) at (72, 260), 486 wide, up to 10 lines, centred
  on 260; race 0x41f "Press [BUTY] for more hints." right-aligned at 560 (bottom 346). Nemesis race (0x818b7 /
  0x81978): race 0x425 "Nemesis Challenge:" instead of "Hint:", 0x426 as the text, no "Press [BUTY]" line, and Y does
  not step the hints (0x8068a). Ported 2026-11-14 (TournamentLogic: 40 hints 0x5dc..0x603).
- Bottom right, right-aligned at 560, bottom 436, 0.7, panel 8 px around (bottom 444): loading: infotext 0x210
  "Loading...", grey (vblanks & 31) x 4 + 128; loaded: race 0x519 "Press [BUTA] to continue." in a highlighted
  panel (0xffa08040), colour (n x 0x11, n x 0x11, 0), n = loader frame & 15.
- Nemesis Challenge intro (0x80a12): the loader start (0x80450) and each frame (0x791f0) set +0x1a84 when the race
  is a tournament's race against the nemesis (Amateur / Expert race 3, Master race 4, Redshift race 0: the races on the
  home track's other route) and the loader's clock t (now - +0x1a80) is below 6 s (0x1a36b0). Then the frame draws
  only the scene transition and race 0x535 "NEMESIS CHALLENGE" in font 1, centred on x (alignment 2, y = the top),
  through 0xb11f0 (scaled) / 0xb1270 (turned: the matrix (cos a, sin a, -sin a, cos a)): two sweeps, x 800 - 700 t at
  y 0 x 14 in 0x60ff4040 and x 1000 t at y -100 x 20 in 0x80ff4040; 16 copies i at (320, 240) in (16 i << 24) |
  0xff4040, before 4 s turned by max(320 r - i max(8 r, 0), 0) deg (r = 4 - 2 t) and scaled max((0.4 i + 2.5)(4 - 4 t)
  + 1.4, 1.2), from 4 s at rest x 1.2; from 2 s the text at rest in 0xffffb040 and an echo at (320, 240 - 12 s) x s,
  s = 1.2 + 20 (t - 2), alpha 160 x clamp(1 - (t - 2)) in 0xffff40. The input skips A / Y while it shows (0x805b5).
  Argon (ui\nemesis_intro.cpp, ui\loader.cpp): as above, the texts turned by rotating their vertices about the
  anchor; `--tourney-race <level> --tourney-done <n> --loader-at <t>` shows it at t.
- Not ported: the multiplayer list (per player "Player n - Press [BUTA] to race." race 0x537 + n / names at
  (72, 234 + 40 n)), the reconnect message (race 0x540 + n). Y (any player) steps the hint: 0x81be0, Quick Race 0x1bbc2c wraps after 31 (32 hints,
  0x618..0x637; 0x637 has no string), Tournament 0x1bbc30 after 39, Time Attack 0x1bbc28 after 15.
- Argon (ui\loader.cpp, the load app_loader.cpp): page 5 of the menu; the track loads after the first frame, then
  A starts the race; Y steps the hint. Like the game's loader thread the screen keeps animating: a worker thread
  reads and decodes everything the race needs (track_source.cpp: models, textures, track data, the route and
  collision grids, the sounds, the grid's craft and the pickups not loaded yet), then the main thread hands it to
  the GPU and the audio engine 4 ms per frame (track_load.cpp), the uploads batched without waiting for the GPU.

- Behind the text: loader_run (0x80090; +0x1a74 = 2 in a tournament, else 0 until 0x804c9 starts the flyby after the
  thread) loads data\movies\<t>\<t>_glory.nms and <t>_racestart.nms (0x95e70) and starts the glory sequence (0x95ef0),
  a looping camera flyby of the track ([sequences/README.md](../sequences/README.md)); the loader thread's frame (0x808e0) draws only the text over it.
  Argon: app\app_sequence.cpp, once the track has loaded (until then the menu background).
- Tournament story scenes ([sequences/scenes.md](../sequences/scenes.md) "Story scenes"): before each tournament race the first pass (0x80354) names the
  scene from the table 0x1b4158 [track x 16 + character]; while the race loads the frame draws only "Loading..."
  (infotext 0x210, font 1, centred at (320, 256), 0x81acb); then the scene's page plays (letterboxed, subtitles, A /
  START skips), and the second pass (0x803b1) shows this screen with the flyby, its clock from 0. After a tournament's
  last race won (0x7fa50) the beat-the-nemesis (Amateur..Master) or winning (Redshift) scene plays before the front
  end. Argon: ui\loader.cpp (Phase::Story), App::playRaceEndScene, movie\story_scene.cpp.
- Race start (init 0x43be0 "Countdown", race_intro_start 0x43d60, race_intro_update 0x43de0):
  - Grid intro (0x52af9d; not modes 3 / 4, only the first race after a load: 0x1b9788, set by the init): the camera
    shows craft (crafts - 1 - floor(t / 2)) for 2 s each, from the end of the craft list to the player; e = 1 - (1 -
    fmod(t, 2) / 2)^3 (0x1033fe = fmod); eye = craft + its matrix x offset, looking at the craft, fov pi x 0.4; the
    first shot (the last craft) from (0, 10, 100) to (2, 2, -4), the others from (-8 / 8, 1, 10) to (-2 / 2, 1, -3)
    (odd index +). A pad's A skips it; then 0x3f1d0 resets the clock.
  - Countdown on that clock: hudmodels\countdown3 / 2 / 1 / 0 ("GO"; _german / _jap / _russian / _arabic) in their
    own full-screen viewport (camera (0, 0, -10) at the origin, fov pi x 0.4). Model k: target T = 1 / 2 / 3 / 4,
    leave L = 2 / 3 / 4 / 4.4; v = T - t before T, (L - t) x 0.4 after L, else 0; turned about y by 600 deg x v
    (leaving also pitched by sin(px v x 11 deg)), at (-px v, -10 v when leaving, -50 v), px = 70 / -70 / 70 / -70:
    on the console each comes in spinning from the distance (left / right) and leaves past the camera. At T the sounds SF_<track>_three / two / one / go
    (0xc38b0 / 0xc3920 / 0xc3990 / 0xc3a00, once each: flags +0x8ec..), from t 1 the race music (music_set_mode(1));
    past 4 every pad gets control (0x78270 -> 0x3510 + pad) and the lap timers start (0x63a30); past 10 off.
    The hud viewport (0xadab0 defaults): fov pi x 0.4, aspect 4:3, near 0.001, far 5000; with the widescreen flag
    (0x298ecc, the dashboard setting) 0xae550 / 0xae5c0 divide the aspect by 0.75 (16:9, same vertical fov). Live
    (xemu, widescreen on): projection m00 0.774 / m11 1.376, right-handed (m23 -1); view diag(-1, 1, -1) + (0, 0, -10);
    the digit matrices match the formulas above. The digits are drawn over everything (the inversion
    shows the ship through them). An arriving digit only shows once it is close (xemu frames: seen about 19 units from
    the camera, not at about 31; not decoded why). Both meshes of every digit have no texture stages, surface flags 0x33
    (two-sided) and blend 0x10800403 (src 1-DST_COLOR, dst DST_COLOR, subtract, translucent pass): the digit inverts
    the picture behind it.
  - Argon (app_intro.cpp): both; until GO the crafts' physics runs with no input (they settle onto their hover
    height) and the lap timers stay at 0, at GO they get control; the countdown is drawn over the whole window
    (vertical fov pi x 0.4 at the window's aspect, no pillarbox) from a camera at z +10 looking down -z, so the digits
    come in from the distance and leave past the camera as in xemu frames of the console; the depth is cleared first
    (on top of the ship and the ground) and a digit farther than 25 units from the camera is not drawn (kCdMaxDepth); the loader's race gets the
    grid intro, Race Again / Restart only the countdown. The intro's labels (each craft's portrait and name, "Nemesis"
    on the player's nemesis) are the nemesis marker's ([weapons/README.md](../weapons/README.md), 0x4d9d0). Only the racing crafts are on the grid (the
    game builds no others, 0x79250); the editor shows a ship on every start slot (`App::slotShown`). Not ported: the music timing (argon's race music starts
    with the race), the widescreen flag elsewhere in the UI.
    Tests: `--intro-at T`, `--countdown-at T` with `--quick-race <id> --race-go`.
