# Quantum Redshift ship effects (from default.xbe)

Ship shadows, trails, water spray and the HUD power-up panel. Ported in `src\main.cpp`
(search for the addresses below). Water itself is in [water/README.md](../water/README.md); the rest of the HUD in [physics/README.md](../physics/README.md) (HUD).

| file | what |
|---|---|
| [pickups.md](pickups.md) | pickups |
| [rain.md](rain.md) | rain |
| [rendering.md](rendering.md) | explosions, flares, camera-facing meshes, blends |
| [sprites.md](sprites.md) | the sprite pass |
| [water-wake.md](water-wake.md) | the water wake |

## Ship shadow (0x9b820 / 0xbe230 / 0x9c380)
- Per ship: an orthographic 13.5 x 13.5 view from `ship - 100 x L` (L = sun direction) renders the ship
  silhouette into a render target.
- Receivers: up to 64 nearby collision triangles that face the light. Vertex grey = clamp(35 - |dy|, 0, 30)
  (dy = height difference to the ship), x the silhouette, projected with the same ortho view.
- Blend ONE / ONE, REVERSE_SUBTRACT (0x800b): the shadow darkens the frame.
- The silhouette is the craft's shadow hull only (surface flag 0x400000, [models/effects.md](../models/effects.md) "Shadow hull"): 0xbe230 draws
  the ship with 0x50e327 clear, so cmm_draw_mesh skips every other mesh. Argon does the same.
- Argon: "Ship shadows" + "Solid" (filled silhouette, additive glow meshes and cutouts left out; the game
  looks filled / blobby). Off = textured ship image.

## Trail (trail.cpp: add 0x9d890 / 0x9da30, fade 0x9db10, draw 0x9dca0; emitter 0x50960)
- 12 trails x 96 points. Per ship two trails at `ship -+ 2 x right - 4 x forward`.
- Each point has two cross sections: +-0.5 x up (vertical) and +-right (horizontal).
- Colour from |forward speed| x 1.1 (max 0xf0; during a turbo, craft +0x9f1, x 2, max 0xff), blue to red; written 3 updates late; every colour
  fades by 1 per update.
- Texture newtrail.dds on stages 0 and 1, additive; v scroll -0.5 / -2 per frame.
- The stage combine is NOT set by the trail draw: it inherits the last translucent mesh's state
  (flush 0xbbf60 -> 0xba660). Argon offers the likely combines ("Trail" look + Gain). Open: none of
  them match the game yet.
- TRAIL / WATERTRAIL mesh effects have callbacks (0x4ff70 / 0x501c0), but no model has those bones.
- Set-up 0x9d890 (neon_load_trail, from neon_app_init_load at 0x775d0; ECX = 96 points, EAX = 12 trails): the
  vertex buffer "Trail vertices" 0x290750 (FVF 0x242: XYZ, DIFFUSE, TEX2; 0x20 bytes: +0 position, +0xc colour,
  +0x10 uv0, +0x18 uv1; 4 x 96 vertices a trail, the first half the vertical cross section, the second the
  horizontal one, a point = a pair -+ its offset), the indices "Trail Indices" 0x290744, data\newtrail.dds; it ends
  with a jump to the reset.
- Reset 0x9d9a0 (trail_reset): every vertex's position and colour 0 (an invisible additive ribbon), the uvs written
  again (uv0 = (0 / 1, slot x 0.25), uv1.u = slot x 0.25 (0x1a3518), slot = the point's index in its trail, both
  cross sections) and the 32 head indices 0x290768 = 0. Callers: the set-up above and the race's effects reset
  0x3f730, which also clears the puff / trail sprite and droplet bitsets (0x1f0330, 0x240538) and the sprite pool's
  transient quads (0xbcf00, "Sprite pool record" below). 0x3f730 runs at the race load (loader_run, 0x8073e), the
  race setup 0x3dc70 (0x3e46e), the restart (gamelogic_update_common 0x3bb45: GAMELOGIC message 7, sub-messages
  2..4), View Replay (0x3e8e0 at 0x3ec7d) and a later viewing's restart from 0 (0x3e700 at 0x3e880; [replays/README.md](../replays/README.md)). So a
  new race or a replay starts with no trail, spray, dust or weapon sprite of the last one. Argon: EngineTrails::reset
  and WakeSpray::clear (from resetDriver: the race setup, the restart, View Replay) clear the trails and the spray,
  resetPickups ->
  resetWeapons the trail sprites; nothing more to port.

## Speed blur (race scene 0x93490, single player, not in replays 0x52afac)
- s = the camera's speed along its view (camera +0x338 velocity . -(+0x32c)); above 200 m/s (720 km/h, in practice a
  turbo) the last frame (echo texture 0x50e16c) is drawn back over the 3D view (fe_draw_echo_quad 0xa6f00) with alpha
  a = clamp((s - 170) x 1.5, 0, 120) and zoomed by a x 0.1 (stored 0x52b04c / colour 0x52b050); then the back
  buffer is copied into the echo before the HUD, so the smear builds up outward.
- With 0x52c8ba set the swirl 0xa7080 (zoom 1 + 0x52b04c, turn 0.2) is used instead, alpha from (s - 50) x 1.4 (0..190,
  + 20, x 3 / 4), at any speed ([frontend/cheats.md](../frontend/cheats.md) "Easter eggs and cheats in a race"). Argon: SpeedBlur::drawHyperBlur.
- Argon: app_effects.cpp drawSpeedBlur, the echo = the menu feedback image, copied each race frame before the HUD.

## Jets (0x50670 setup, emitter part of 0x50960)
- Setup (all ships): bones whose name contains "JET" (list +0x34 / +0x38) and "JFLARE" (+0x58 / +0x5c); size +0x7c =
  +0x80 = 0.1 (idle), +0x84 = 0.5 (full throttle), +0x88 = 1.0 (turbo), rate +0x8c = 10.
- Per frame: target = (+0x84 x throttle + (100 - throttle) x +0x80) x 0.01, or +0x88 while +0x9f1 (turbo); size +=
  (target - size) x 10 x dt, set to the target at once when that is below (unless it is the idle 0.1), at least
  0.1. The JET bones are scaled (1, 1, size) (0x515c0 -> 0xb9af0); the instance tint (+0x104) of the JET bones =
  (A 255, R 128 s, G 128 s, B 400 s) and of the JFLARE bones twice that (0x51480 / 0x51520), clamped to 255. With
  surface setup 2 (0xba1d0; setup 1 = 0xba370, the same with surface +0x40) and flag 4 (on a bone record) that colour (surface colour
  +0x10 x instance intensity +0x138 (occlusion) x tint, per channel >> 8) becomes the scene ambient (0xa8410: D3DRS_AMBIENT
  cache 0x1188ec, NV2A 0xa10) without flag 0x100, or light 0's diffuse (0xa82d0) with it. So it only shows on lit
  meshes: the jets have flag 0x20 (lighting off) and keep their own texture x vertex colours (per ship: Tara white /
  blue, Alicia and Jordan orange, ...); the flares (flags 0x72855, lit, vertex colours black) are lit by that
  ambient alone.
- The turbo glow at the nozzles is the flares: at full turbo the tint is white, so they show their surface colour.
  Tara: side flares JFLARE10 / JFLARE8 0xffff9100 (orange), middle JFLARE2 0xfffff7d2 (white); Abi 0xff06fff3
  (cyan); Alicia 0xffffff8d. Idle (size 0.1) they are at ~10 % (blue ~30 %).
  The other lit flares without a jet tint (Tara's FLARE9_*, 0xff4e9fff) use the default tint 0xffffffff.
- Argon: effects\craft_exhausts.cpp updateJet, app_frame.cpp (the scale in the jet bone's space; the tint in the model
  matrix's bottom row), mesh.vert / mesh_shading.glsl vTint / vTintLit (the diffuse = surface colour x tint, vertex colour
  ignored). The intensity +0x138 is the flare's occlusion test ([models/README.md](../models/README.md), surface flag 0x40000): 1 when at least
  half its points are visible, fading as the hull hides the nozzle, not drawn when hidden.

## Dust (craft_hover 0x5e6d0 -> trail sprites 0x9d000)
The hover raises it, in every physics substep (0xcf630: 4..20 ms, so more substeps and more dust the faster the
craft), for every craft (player and drones, no player check), never in a replay (0x5f367: not while 0x52afac). The
rays are the rear box corners 2 and 3 (the same corners as the water wake):
- 0x5f377 / 0x5f4f1: the surface of ray 2 ([ebp-0x98], from the collision table's +0 for the hit mesh, 0x5f171) /
  ray 3 ([ebp-0xa0], 0x5f1fb) must be 1 (rough) or 2 (loose). Both start at -1 (0x5e6e0): a ray that missed, hit a
  wall (|n.y| < 0.2) or only found water has none; surfaces 0 (track), 3 (water), 4 (slippery) raise nothing.
  Nothing else gates it: no speed threshold, no "on ground" (3 contacts) check, one rear ray is enough.
- 0x5f38b / 0x5f505: one rand() per such ray, dust when rand & 0xf == 0 (1 in 16).
- Position: the ray's hit point (collision +0x81cc, kept per ray at [ebp-0x13c + 12 k]).
- Velocity: (forward.x x -10 + ((rand & 0xff) - 127) x 0.0125, 1.5, forward.z x -10 + ((rand & 0xff) - 127) x
  0.0125), forward = craft +0x1f0 (0x1a372c = -10, 0x1a3538 = 0.0125): 10 m/s back along the craft, not its speed.
- Colour (0x5f35b..0x5f466): base 0xb0a070 / offset 0x302816 (sandy), on Mount only (track id 6 at 0x52ccf8; not
  alt_mount) base 0xb0b0b0 / offset 0x303030 (grey); per channel byte((base x ground >> 8) + offset), ground = ray 3's
  vertex colour (0xa46a0, [ebp-0x24], written only by ray 3 at 0x5f24e; both rays read it). Alpha =
  (rand & 0x3f) + (rand & 0x1f) + 40 (40..134). The end colour is the same with alpha 0.
- 0x9d000 (EAX = 0: quadsprite.dds cell 0, the soft cloud): life 2.5, fade 0.5, half size 2 + (rand % 20) x 0.1
  (2..3.9), growth 1.08 (both axes), start angle rand % 360 degrees (+0x234), spin 40 + rand % 60 degrees per second
  (ray 3: negated, 0x5f5fd), list flag 0 = list A alpha blended, fade-in 1. So 0x9c540 fades its alpha in over 0.5 s,
  then out to 0 over the last 2 s; the quad turns to start angle + spin x age (0x9c9d0, x 0x5a211c degrees to radians).
- The rand order: roll, drift x, drift z, alpha (2), spin, angle, size, then 0x9d000's own.
Argon: logic\hover_dust.hpp (the rules), GameCraft::raiseDust (the rolls in hover(), per substep, into
GameCraft::dustPuffs), App::raiseDust (the sprites, player and drones each 60 Hz step) into the trail sprites
(app_trail_sprites.cpp; they now age every step, weapons on or off). Port choices: when ray 3 held nothing the game
reads stale stack for ray 2's colour; argon uses ray 2's own. Not spawned while the race is held on the grid (the
argon's effects clock stops there). The roll uses the craft's rand(), the rest the effects' (one rand() in the game).
Tracks with rough / loose collision meshes: all but Mars / alt_Mars (counts of collidable meshes, surface 1 / 2):
silver 41 / 49, kalau 28 / 0, fuusoku 14 / 0, flames 25 / 6, solar 22 / 4, bracken 56 / 1, mount 47 / 34, draco
12 / 4 (the alt_ routes about the same).
