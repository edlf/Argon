# Effects: explosions, flares, camera-facing meshes, blends

## Weapon explosions (0x7e0f0 / 0x7de80; [weapons/explosions.md](../weapons/explosions.md) "Explosions")
- An explosion is a model, objects\explosion_type0..15 (16 copies each, 0x7dd40), not sprites or particles: drawn
  for its life at the impact (scaled, turned at random by its type), its bones' tracks run once over the life (their
  own clocks, progress x the type's clock factor), every bone tinted a grey falling from 254 to black (the tint shows
  on lit meshes only). Camera-facing meshes (the explosion grids, surface flag 0x800; type 9's also 0x100000:
  "Camera-facing meshes" below) face the camera through the render list.
- Its flash when it goes off and the impact's flash (burst flag 4) are trail sprites (0x9d000) in quadsprite.dds cell
  15, additive, white to transparent black over 0.5 s, shrinking (growth 0), unturned: half size the explosion's scale
  x its type's flash factor (up to 2000 m: Kumiko's and Kutoo's specials) or the weapon's area inner radius (at
  least 10). A big one near the eye covers the view.
- The impact's sparks are point particles: 250 starburst on a craft hit (3 m/s, 3..3.5 s), 50 starburstred on every
  impact but a bounce (10..30 m/s upwards, 1..1.25 s).
- No light, no screen effect of its own; the shake is the push's ([physics/rumble.md](../physics/rumble.md) "Rumble").
- Argon: logic\explosions.cpp, weapon_explosions.cpp, SceneRecorder::drawExplosions, logic\explosion.hpp.

## Flare occlusion (cmm_draw_mesh 0xb6e60, test 0xa64f0, result 0xa6750; [models/README.md](../models/README.md) surface flag 0x40000)
Every cmm mesh drawn without a depth test is a "light": surface flag 0x10000 (0xba575: D3DRS_ZENABLE 0 through
0xa86b0, the depth mask left as it was) always comes with the test flag 0x40000 or the copy flag 0x80000 in the
game's files (every .cmm checked). They are the tracks' lightglow_* quads (16 vertices, flags 0x2070871, additive
0x10007: the sorted translucent bucket), the sun's FLARE_ mesh and its lens pieces c1..c5, the big revol / gtf / grid
halos (Mars, Kalau, Mount), the crafts' JFLARE / FLARE / revol meshes (flags 0x72855 / 0x7083f).
- The test points: 0xa63b0 at device set-up fills a 256-point vertex buffer (0x292d40, stride 0xc): x, y, z =
  (rand & 0xfff) / 4095 (0x1a3568), each negated on its own rand & 1, kept when the length (table square root) is at
  most 1. rand is the CRT's (0xf96ea: x 0x343fd + 0x269ec3); srand(time) is called at 0xc463a / 0xdedfb, so the set
  is not fixed. Argon uses a fixed set with the same distribution.
- The test (0xa64f0): world = the mesh's world matrix (0x50e230) with its three axis rows x the surface radius;
  blending off (0x40304), colour mask 0 (0x40358), blend colour 0 / factors CONSTANT_COLOR, alpha test off (0xa94e0),
  depth write off (0xa8640(0)), depth test on (0xa86b0(1): ZENABLE = 2, the w-buffer), then a visibility test
  (0x10d1e0 / 0x10d280, index +0x6040 cycling 0..0x3fff) around a point list of the first N points (0x108d90). The
  depth buffer holds everything drawn so far (the track's lights are in its translucent bucket; argon tests
  them after the opaque track, the crafts' opaque meshes and the water). The mesh's own state (0xba4f0, the no-depth
  draw) is set only after the test.
- Radius / N (surface lighting +0x0c / +0x10): lightglows 1.0 or 4.0 m with 4 points (2.0 on some of Silver's),
  suns 100 / 400 m with 256, revol halos 50 m / 32, gtf 5 m / 16 or 32, Mount's Little_Flare 5 m / 64, the crafts'
  JFLARE 0.3 m (Tara 0.1) / 32, revol 0.5 m / 32.
- The intensity (0xb70ba..0xb7187): visible = the result of the bone's last test (+0x108; 999999 = none yet, set at
  load by 0xb9540: visible stays 0), +0x138 = min(1, 2 x visible / N), the tint x +0x138 -> +0x10c; then the new
  test's index -> +0x108. So the result is the previous frame's, and a light is not drawn the first time its bone is
  tested. 0xb71f8: a bone whose +0x138 <= 0 skips the draw (a 4-point lightglow shows with one point visible, at
  full colour: it is unlit, the intensity only scales lit meshes' colour).
- Argon: SceneRecorder::testedOcclusion / queueOcclusionQuery, FlareQueries (render\flare_queries.hpp; Vulkan occlusion queries, read
  when the frame slot comes round again; precise sample counts / samples per pixel), render\flare_occlusion.hpp (the
  rule, flare_occlusion_tests). It used to draw a light at full intensity until its first result came back: each
  time a sector's lights first came into the visibility set ([tracks/README.md](../tracks/README.md) visdata) they showed for two frames through
  hills and walls (Draco's chain of glows behind the hill after the start, ~20 at once). Not ported: the split
  screen views share each bone's result (as the game's one +0x138 per bone; its order of reads and writes between
  the views is not followed).

## Camera-facing meshes (surface flags 0x400 / 0x800 / 0x100000; render list 0xb7920, cmm_draw_mesh 0xb6e60)
Three surface flags place a mesh by the camera, in two steps.
- The render list (0xb7920) gets the eye (arg 4: 0x50e308, the position 0xa7900 stores beside the view matrix
  0x50e1f0) and, from the models' bone-by-bone draw (0xb83e0 at 0xb84d8 / 0xb85ff), arg 6 = 1; a single bone's
  draw (0xba100: the shots, 0x6ca06) passes 0 and uses its own matrix untouched. With arg 6 and surface flags &
  0xc00 (0xb7d5f; not for meshes with flags 0x44000000, drawn with an identity matrix) it copies the bone's world
  (+0x40, posed with the instance by 0xb9210) and: 0x400 (0xb7e88) adds the eye to its translation (the bone holds
  an offset from the camera: the sun and lens pieces of Fuusoku, Draco (both routes) and alt_solar, ~2800 m away, so they ride with the
  camera like the sky); 0x800 without 0x100000 (0xb7ec2) turns it to the eye (0xb4e70). That matrix is what
  cmm_draw_mesh sets as 0x50e230, so the flare occlusion test (above) uses it.
- 0xb4e70: D3DXMatrixLookAtRH (0xf3150) from the mesh's position to the eye, up (0, 1, 0), its 3 x 3 transposed into
  the world rows: z = normalize(mesh - eye), x = normalize(up x z), y = z x x. The mesh's rotation and scale are
  replaced (unit axes). The flagged quads lie in their local x-y plane, so they face the camera.
- cmm_draw_mesh with 0x100000 (0xb7765, after the occlusion test): factor = the surface lighting +0x14 (0xbb270:
  setup 1 at +0x54, setup 2 +0x24); axis = the view's third column (0x50e1f0 [2], [6], [10], normalized by 0x58360);
  d = position - eye; along = axis (axis . d); position = eye + along + (d - along) x factor; then with 0x800
  (0xb78a1) turned to the eye from there. So a piece keeps its depth and slides towards the screen centre: factor 1
  in place, 0 at the centre, negative across it. The draw uses a copy: the bone keeps its matrix.
- Who has them (all .cmm files): 0x100000 always with 0x800. The sun's lens pieces c1..c4 (c5 on Flames) under the
  FLARE_ bone of Fuusoku, Solar, Draco, Flames, Bracken and their alt_ routes (9-vertex quads 157..336 m wide,
  setup 1, flags 0x1bacb5 with 0x400 / 0x1ba8b5 without (Solar, Flames, Bracken); factors -0.75, -0.8, -0.2, 0.2, Flames -0.5, -0.9, 0.2,
  -0.2, 0.5; intensity from the FLARE_ mesh, flag 0x80000), Darius's grid2..grid4 meshes and explosion_type9 (0.5 /
  0.2 / 0.8). 0x800 alone: the sun's FLARE_ mesh (Fuusoku: a 3079 m quad; occlusion tested; 0x400 as its pieces),
  ~1500 track lightglows, the crafts' JFLARE / revol / LGREN meshes, explosion grids, the front end ship models'
  flares and frontend\earth's EARTH mesh (flags 0x803). The lens pieces' and the sun's vertex colours are black and
  they are setup 1 with bit 15 (lit) and flags 0x2000 (light 0 black) + 4: their colour is the ambient they set, the
  lighting block's white x the sun's occlusion intensity ([models/surface-block.md](../models/surface-block.md) "Surface block", [models/effects.md](../models/effects.md) "Lighting"), times their
  additive texture. Argon: render\d3d_lighting.cpp.
- The camera step is only in the render list's deferred branch: 0xb7c72 tests the current render list (0x298ee0);
  with none it sets the bone's world (+0x40) at once (0xa78c0 -> 0x50e230) and calls cmm_draw_mesh (0xb7cdc), so
  0x400 / 0x800 do nothing there (0x100000's own step in cmm_draw_mesh still runs). The race scene sets its list
  (0x93e7c) and clears it after (0x93fba); the front end sets lists in some draws (0x59e79c at 0x13624 / 0x161a4 /
  0x333c7, 0x59e7e8 at 0x2a73d / 0x33521 / ..., its own at 0x18d50) and never clears them, so which branch the
  globe and the ship models take there is not settled from the code.
- Argon: render\camera_facing.hpp (CameraPlacement::listed / drawn, camera_facing_tests), Draw::camera,
  SceneRecorder::listedWorld (the render list step; ModelPlacement::onlyBone draws and the front end's models skip
  it: ModelPlacement::cameraListed), drawMesh (the draw step), queueOcclusionQuery (tests the listed matrix),
  useViewPoint (each view's camera). The look-at with the eye straight above or below the mesh (zero x axis) leaves
  the matrix as it was (the game's normalize of a zero vector is not followed).
- Open: the front end. The course select's globe (0x1d360 -> 0xb83e0(matrix, 0, 1, eye 0x5a1df8) -> 0xb9210:
  bone +0x40 = local x the turned, 0.9-scaled instance; -> 0xb84d8: render list arg 6 = 1) has EARTH with surface
  flags 0x803 and mesh flags 0x121 (not 0x44000000). Through a render list the step would replace the turn and the
  scale with the look-at frame and the globe would always show the same face; in the game it turns to each track
  (the brackets mark the track on it). Settled live (2026-11-16, xemu, the course select's globe turning): at the
  globe's draw call 0x1d59b the render list 0x298ee0 is 0 every frame, so the globe takes 0xb7c72's direct branch
  and keeps its turn and scale; argon's front-end models skip the list step (ModelPlacement::cameraListed) as
  the game does. (The ship models' JFLARE / revol flares on other front-end pages were not read.)

## Blend modes (neon_setup_render_state 0xa9530)
0 off, 1 alpha, 2 additive, 4 reverse subtract.

## HUD power-up panel (element +0x3950, draw 0x4eb20)
- 128 x 64 at ((L + R) / 2 - 64, T).
- Layers: weaponbase (mode 1), then per colour red / blue / yellow level 1..3 `weapon<colour><n>` (mode 2,
  additive), then weaponspecial (mode 1) when the special is held.
- Textures held at 0x1be970, 0x1be998, 0x597cd0 (stride 0x28).
