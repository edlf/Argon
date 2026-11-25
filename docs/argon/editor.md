# The editor and debug tools

What `argon_debug` adds to the game: the track editor (FE MAIN's Debug row, or `--editor`) with its control panel, and
the command line. The game itself (front end, races, menus) is described in [frontend/README.md](../frontend/README.md),
[physics/README.md](../physics/README.md) and [weapons/README.md](../weapons/README.md).

## The editor's panel
- Track combo switches tracks (all 18 load in < 0.1 s).
- **Layers:** collision meshes (road/walls), invisible walls (off by default),
  visual-only meshes, bone pieces (animated/effects), sky, water.
- **Markers:** start grid (green arrow = slot 0), AI racing line (yellow,
  orange ticks = nodes, tall = respawn allowed), pickups (colour by type).
- **Render:** textures, lighting and fog from `prop.dat`, **Game materials**
  (the game's own per-mesh blend mode, alpha test, depth write and culling —
  glows are additive, decals/billboards alpha-blended in a second pass), wireframe.
  With Game materials off everything is opaque and an Off/Auto/All cutout guess is used.
  Anti-aliasing: Argon Options' MSAA, 4x by default (`--msaa N`, shown next to Back-face culling), and its Post AA
  (FXAA or SMAA over the 3D view, `--post-aa`); opaque alpha-tested
  cutouts (fences, foliage) use alpha-to-coverage with a sharpened alpha edge so their cut edges are
  smoothed too (blended cutouts keep the game's plain alpha test).
- **Ships & pickups:** the craft models parked on the start slots (slot 0 = "Your
  ship", the rest in pilot order; shield/weapon effect parts hidden; in a race from the menus only the racing crafts) and the real
  pickup models (power chip, mega-up, red/blue/yellow power-ups) spinning at their spots.
- **Audio** (miniaudio, with a mastering limiter against clipping; L/R announcer
  files are merged into one stereo voice): the track's music playlist, 5.1 ambience (the story scenes' only, as the game),
  positional emitters from `audio\tracks\<t>\<t>.txt` (crowds, birds, trees) that fade
  with distance and pan with the camera, your ship's layered engine (both sets) with an Engine
  slider, the 3-2-1-GO countdown and the other one-shots. "Show emitter markers"
  draws each emitter with its MINDIST/MAXDIST rings. Format notes: `docs\audio\README.md`.
- **AI drive:** Start lets the game's own AI pilot drive your ship (slot 0) with the
  game's own physics, ported from default.xbe (`src\craft\craftsim.cpp`, notes in
  `docs\physics\README.md`): 4-point hover springs over the collision mesh, gravity, engine
  thrust, gears/revs, steering, side friction per surface (track/rough/loose/water/slippery
  from the meshes), aerodynamics, water buoyancy, the rigid-body integrator with the game's
  substeps, wall contact response, and the AI's look-ahead targets / steering / throttle
  logic, all from `config\character_stats.txt` for the chosen pilot. Speed class 1-5
  (`character_stats` *1..*5) and AI engine power % (catch-up value, default = quick race
  from `aivalues.txt`) are selectable. Stats: speed, gear, revs, throttle, surface, steering,
  lap / last / best, wall hits, respawns (the game's rule: moved < 5 m in 2 s). The engine
  sound uses the game's mapping (0xc5200): speed smoothed by 1 m/s per frame, as % of the
  class top speed, driving both the `engine` and `h_engine` layer sets like the game does
  for the player's ship. Chase camera views (View list, `--view N`):
  0 **Cockpit** (the game's internal tripod 0x5aa10: camera_up / camera_forward, head tilt from
  the steering with turn_headtilt_*; own ship hidden), 1-3 **Chase close / mid / far** (the
  game's external tripods 0x5be00 / 0x5c700, fov 60), 4 **TV** (the track's replay cameras from
  `config\camera_<t>.txt`: static / moving / heli, each active in its lap-progress window
  `panrange`; static ones track the ship, DYNAMICFOV zooms; the choice between cameras is the
  argon's own), 5 argon chase, 6+ the track's **ship-mounted** cameras (ship space x forward,
  y left, z up). Ship and camera are drawn interpolated between the 60 Hz physics steps.
- **HUD** (tick "HUD", `--nohud`): the game's own HUD layout, decoded from gamehud.cpp (notes in
  `docs\physics\README.md`, HUD): the game fonts (Square721 18 / 34) and `race_strings.str` labels, lap, time / prev /
  best (MM:SS.hh), shield, the speedometer with the turbo box, the power-up panel, the RESET / WRONG WAY messages
  and the rotating minimap with the position list (`data\hud\minimap\<t>.dds`; not every track has one). Placed on
  the game's 640 x 480 safe area, widened for wide windows. **F1** (or `--nopanel`) hides the control panel.
  Right mouse takes the camera back (re-tick Chase camera).
- **Viability check** (runs on every track load, also printed to the console): walks the
  route over the collision mesh and lists *problems* — no road under the line (red
  pylon), solid wall across it (orange), a ledge where the road jumps up > 2.5 m (yellow)
  — and *jump/drop* sections where the line is > 8 m above the ground (blue, info only).
  "Go" flies the camera to an entry. All stock tracks pass or have 1-4 spots to look at.
- **Camera:** hold right mouse to look, WASD move, Q/E down/up, Shift ×5,
  mouse wheel changes speed. "Top view" turns fog off (it whites out at altitude).

## How it matches the game
- Meshes draw in two passes like `cmm_draw_mesh` (XBE 0x000b6e60): opaque in
  file order, then water, then the translucent pass sorted far to near like the race's render list (0xbb820; a
  mesh with surface flag 0x20000 last of all), each translucent mesh lifted a hair towards the eye so a layer on
  its ground does not flicker. Argon Options' Translucency (Order Independent by default, `--oit on|off`) blends
  the alpha-blended ones without the order ([differences.md](differences.md) "Translucency").
- Blend factors/equation come from the surface block's blend descriptor
  (decoded in `src\model\material.hpp`, documented in `docs\models\README.md`).
- Up to 4 texture stages per mesh, combined exactly like the game's fixed-function
  setup (COLOROP/ALPHAOP per stage, argument order, PREMODULATE), with UV sets,
  UV scrolling, env-map (reflection) texcoords, clamp/wrap and animated textures
  (frame arrays). Env-map stages use the game's texture matrix (world-space reflection), so
  reflections stay put when the camera turns. That's what drives the "GO" screens, gloss reflections, layered
  terrain and scrolling/animated signs. Stage 0 starts from the vertex colour
  (prelit meshes) or a sun+ambient lighting stand-in.
- No axis mirroring: the data is used as right-handed with clockwise front faces
  (earlier versions mirrored the world).
- Water: the game's own water (`docs\water\README.md`): 128 m patches over the `water.dat` boxes the game
  draws (only Fuusoku has any; the other boxes are physics only and their water is the track's meshes),
  30 precomputed wave frames blended like the game, the 60 `waterbump` frames, the waterbumpfresnal vertex
  shader and the texm3x3vspec pixel shader ported to GLSL (bump-mapped cube-map reflection x (0.5, 0.7, 1.0),
  Fresnel alpha 0.6..1).

## Command line
All but `--game` need the developer build (`argon_debug`, see [argon/build.md](build.md)).
```
argon --track solar                       start on a track
argon --validate                          Vulkan validation layer on (messages to stderr)
argon --track mars --shot out.png --cam top    render a few frames, save PNG, exit
         [--frames N] [--campos x y z yaw pitch]  (game coords, degrees)
         [--look x y z] [--emitters] [--sound]           (--sound keeps audio on for --shot)
         [--nomaterials] [--alpha off|auto|all] [--nocull] [--nosky] [--size W H]
argon --pilot zafir                       pick your ship (slot 0 + engine sound)
argon --drive                             start the AI drive at launch
argon --track mars --drive-time 60 --shot out.png   simulate 60 s first (shot from the chase cam)
         [--speed 1..5] [--drive-log N]          speed class; print the craft state every N s
argon --nopanel                           start with the control panel hidden (F1 toggles)
argon --msaa 8                            anti-aliasing samples 1/2/4/8 (default settings.json's, capped to the GPU)
argon --post-aa smaa                      post-process anti-aliasing off/fxaa/smaa (default settings.json's)
argon --oit off                           translucency on/off: Order Independent or the game's order (default settings.json's)
argon --renderer vulkan|d3d12             the graphics API (default: settings.json's, else vulkan)
argon --mute                              no audio
argon_decode in.wav out.wav              decode a game WAV (Xbox ADPCM), FLAC or Ogg Vorbis to PCM (prints its play length and loop)
```
These are the common ones. The test options (start on a menu page, jump into a race, a replay, a race end, the
tournament, multiplayer, ...) are parsed in `src\launch\launch_options.cpp`, grouped by what they start. Where the
viewer starts: `--menu` (no value) the front end, `--editor` the editor; else a test race (`--quick-race`, `--replay`,
`--tourney-race`) starts in the front end and goes through its loading screen, and `--track`, `--drive`,
`--drive-time` or `--shot` start in the editor. Before 2026-11-22 a test race without `--menu` started in the editor
instead when one of those was given: the AI drove the editor's track (bracken unless `--track`), no race was loaded
and `--egg` did nothing. In the front end `--drive-time` does not step the race before a shot; `--countdown-at 0
--frames N` lets it run that many frames.

## Known limits
- Approximations: detail normal maps (texture type 3) are shaded with a fixed
  light direction; D3D vertex lighting for unlit meshes
  is a sun + ambient stand-in; texture factor (BLENDFACTORALPHA) assumed 1.
- Audio: emitter fade uses MINDIST = full volume, MAXDIST = silent (inferred);
  no reverb (reverbs.txt / NOREVERB not applied).
- Physics: the hull contacts' triangle search uses argon's collision grid in place of the game's span lists
  ([physics/ai-and-port.md](../physics/ai-and-port.md) "Port").
- Viability check thresholds (8 m, 2.5 m, 60° walls) are guesses.
- Read-only: the editor shows a track but does not write it back.

Everything else still open is in [argon/todo.md](todo.md).
