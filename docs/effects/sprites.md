# Effects: the sprite pass

## Sprite pass (0xbd8f0)
List A = alpha test + blend mode 1 (alpha), list B = mode 2 (additive). The stage combine is not set
(inherited), so whether alpha is texture x vertex or vertex only is open ("Vertex alpha" in argon).
- Pools: blended 0x425058 (2000 quads, used flags 0x42a6d0, free index 0x439d9c, count 0x439da4), additive 0x426044
  (0x800 quads, 0x425830, 0x439d98, 0x439da0); 0xc0 bytes a quad = four 0x30-byte vertices: +0 pos, +0xc corner
  offset, +0x18 D3DCOLOR, +0x1c uv, +0x24 sin / cos of the angle, +0x2c s16 a0 (= frame x 3). Transient quads (spray,
  trail sprites) are freed by 0xbd5f0; sprite.dat's (0x41fa28 / 0x426048 = 0) stay.
- Called by the race scene 0x93490 at 0x938e8 (after the ship shadows, the translucent flush 0xbbf60 and the trail
  ribbons 0x9dca0; before the particles 0xbc900) with the camera's view (+0x220) and eye (+0x36c), and by the NMS
  scene draw (0x98e12, when 0x439f62 is set).
- List A (0xbcfa0): used blended quads whose largest axis distance |dx|, |dy|, |dz| from the eye is under 1500
  (0x1a3700), bucketed by that distance rounded (max 4999) and drawn from bucket 4999 down to bucket 1 (bucket 0,
  within 0.5, is skipped), each bucket last added first: far to near. List B (0xbd1b0): used additive quads with
  squared distance under 2.25e6 (0x1a34b8), pool order.
- State: world = identity, depth test on (ZEnable 2, w-buffer) without writes (0x4035c = 0), no culling, alpha ref 0,
  the track fog (neon_setup_fog); list A fog colour = the fog colour with alpha 0, list B fog colour 0 (0xa8610) and
  blend ONE / ONE / ADD. Texture: the track's envdata\quadsprite.dds (512 x 1024, set by 0xbceb0 at the track load).
- quadsprite.xvu: a0 = v6.x; the offset's x / y turned by (sin, cos) (x sin + y cos, y sin - x cos), then dotted with
  c[a0 + 12..14]; + pos; oPos = c0..c3, oFog = c32 (view depth), oT0 = uv, oD0 = colour. 0xbd8f0 uploads (0xaa900:
  the matrix's columns) c12 = identity, c15 = (normalize(view x axis with y 0), (0, 1, 0), normalize(view z axis with
  y 0)), c18 = transpose(view): frame 0 world offsets, 1 upright billboards, 2 camera-facing. The game's view x axis
  points to the screen's left (its projection is mirrored, [models/README.md](../models/README.md) "Coordinate system").

## Sprite pool record (allocate 0xbd670)
`int __cdecl 0xbd670(char pool, vec3 *pos, float halfSize, D3DCOLOR colour, float cellX, float cellY, float cellSize,
float angleDegrees, u8 sprayFlag)`, EAX = the basis row (0 world, 1 upright, 2 camera facing). Returns the quad's
index in its pool, or -1 when the pool is full (the caller then spawns nothing: 0x9d000 / 0x9d2c0 / 0x9d620 return
false, the weapon trail loop 0x66650 stops for the frame).
- pool 0 = additive (list B): quads 0x426044 (0x800), used flags 0x425830, one-frame flags 0x41fa28, free index
  0x439d98, permanent count 0x439da0. pool != 0 = blended (list A): quads 0x425058 (2000), used 0x42a6d0, one-frame
  0x426048, spray flags 0x425060 (sprayFlag; additive quads have none), free index 0x439d9c, permanent count 0x439da4.
  The users' "list 0 / alpha" (0x9d000 / 0x9d2c0 param 11 == 0) is passed as pool 1.
- A quad = 0xc0 bytes, vertices v0..v3 of 0x30 (drawn v0 v1 v2 / v1 v2 v3):

  | Offset | Type | Written by 0xbd670 | Rewritten by |
  |---|---|---|---|
  | +0x00 | vec3 | position (all four the same) | 0x9c540, 0x9ca50: the sprite's position each frame |
  | +0x0c | vec3 | offset: v0 (-h, -h, 0), v1 (h, -h, 0), v2 (-h, h, 0), v3 (h, h, 0) | 0x9c540 (W, H; mode 1 v0 (-W, H), v1 (W, H), v2 (-W, 0), v3 (W, 0)); droplets: v0 / v1 -+ side (0x9d2c0), v2 / v3 streak -+ side (0x9ca50) |
  | +0x18 | D3DCOLOR | colour | 0x9c540 / 0x9ca50 each frame; 0x9d000 / 0x9d620 zero it after the allocation |
  | +0x1c | 2 x float | uv: u = cellX / 512 + cellSize / 1024 -+ 0.97 cellSize / 1024, v = cellY / 1024 + cellSize / 2048 -+ 0.97 cellSize / 2048 (0x1a35bc, 0x1a35b8, 0x1a35b4, 0x1a35b0); v0 (u0, v0), v1 (u1, v0), v2 (u0, v1), v3 (u1, v1) | never |
  | +0x24 | float | sin(angle x pi / 180, 0x5a211c) | 0x9c540: cos(a); 0x9d2c0: cos 0 = 1 |
  | +0x28 | float | cos(angle) | 0x9c540: -sin(a); 0x9d2c0: -sin 0 = 0 |
  | +0x2c | s16 | EAX x 3 (quadsprite.xvu's a0: c[12 + a0]) | never (+0x2e unused) |

  a = (now - born) x spin + start angle (0x9c540: +0x230, +0x234). quadsprite.xvu turns the offset by the two slots
  (s0, s1) = (+0x24, +0x28): (x s0 + y s1, y s0 - x s1, z). With 0x9c540's (cos a, -sin a) that is a turn of x
  towards y by a; along the camera basis (x = the game's view x, to the screen's left) a positive spin turns the quad
  clockwise on screen, and at a = 0 the cell shows upside down and mirrored (v0, its top left texel, at the screen's
  bottom right). 0xbd670's own (sin, cos) is kept only by quads nobody updates: sprite.dat's 90 degrees (unturned)
  and the weapon flashes (a random angle, the cell turned by it).
- Lifetime: 0xbd670 sets the used flag, clears the one-frame flag (and sets the spray flag), then moves the free index
  to the first unused slot at or after the new one (-1 when none). A quad is freed by clearing its used flag and
  lowering the free index to it (when lower, or when it was -1): 0x9c540 at the sprite's end of life, 0x9ca50 for the
  droplets, 0x9d000 / 0x9d620 / 0x9d2c0 when their 4096-entry ring (next index 0x439f58 for the puffs and trail
  sprites, 0x439f5c for the droplets) comes round to a slot still in use (the oldest goes). One-frame quads (the
  caller sets 0x41fa28[i] = 1 after the allocation; only the weapon flashes below) are freed by 0xbd5f0, which the
  main loop calls each frame (0x7f4b3) before particles_update 0x9ca50: they are drawn in the frame that made them.
  0xbcf00 frees every quad from the permanent count up (the race's effects reset 0x3f730; the track load's texture
  0xbceb0 sets both counts 0 first); 0xbdf40 adds sprite.dat's quads below it.
- Callers: weapon trail sprites and dust (0x9d000 at 0x9d14e: EAX 2, cell (128 (t & 3), 128 (t >> 2)), 128 px,
  angle rand & 0x1ff, spray flag 0), droplets (0x9d2c0 at 0x9d416: EAX 0, size 1, spray flag = param 11), puffs
  (0x9d620 at 0x9d6f2: EAX 1, pool 1, cell (0, 384), spray flag 1) and the weapon flashes, all additive, EAX 2,
  quadsprite.dds cell (256, 256) (cell 10), 128 px, angle rand & 0x1ff, one-frame:
  - sparks 0x68150 (0x68311): half size 10 + (rand & 3) (0x1a35a4; rand for the angle first), grey (k, k, k, 255),
    k = clamp(fistp(param 4 x 255)); besides its neon_add_particle sparks (starburst / starburstred, life 1) and, on
    the beam's first frame, a 0x9d000 trail sprite in cell 10 (EAX 10, additive, 1 s white to black; the beam's own
    after a hit at 0x68fde / 0x69245 too). Its only callers are the beams' ([weapons/shields-beams.md](../weapons/shields-beams.md) "Beam and spark flashes").
  - beams 0x68620 ([weapons/shields-beams.md](../weapons/shields-beams.md) "Beam and spark flashes" has when): the start glow (0x68df5, half size 2.5, the beam's
    grey), the sparks' flash and the hit flash (0x68f0a / 0x69171 / 0x6953f / 0x696c9, half size 25, white); in the
    race-over branch 0x687cf (the glow at bounce points only) and 0x688bb (the hit flash).
- Argon: sprite_pool.hpp (the bases, the turn, the corner offsets, the uv cell) for the trail sprites and dust
  (app_trail_sprites.cpp: they had turned the other way, the cell upright), the puffs (upright) and the droplets
  (their cell the right way up), sprite.dat's quads and the weapon flashes (SpriteTurn::ofAllocation keeps 0xbd670's
  own slots; logic\weapon_flash.hpp, App::flashBeam, drawn after the additive trail sprites, cleared at each weapon
  step; the sparks' and the beam's 0x9d000 sprites join the trail sprites, App::throwSparks). Not ported: the shared pool limits (argon caps puffs and droplets at 4096 each and trail sprites at
  2048; the game's 2000 blended quads hold sprite.dat's blended ones, every puff, droplet, dust and blended trail
  sprite together, so a crowded wake can starve them), the free order (lowest free slot first, only what list A's
  bucket order shows). No record header in src\original: argon reads no pool memory, only files.

## Track sprites (sprite.dat; quadsprite_AddCMMSprites 0xbdf40)
- The track load (track_env_load 0xaf910) loads envdata\quadsprite.dds (0xbceb0) then sprite.dat (0xafade ->
  sprite_dat_fixup 0xaf320 -> 0xbdf40). Each record becomes one permanent pool quad (flags bit 0: additive pool,
  else blended); "Late call" error if either pool is not empty, "Too many ..." past 2000 / 0x800.
- Vertices v0..v3 = pos and the record's corners 2, 3, 1, 0, colours +0x44.., uvs (u, v x 0.5 + 0.5: the atlas's lower
  half), a0 = frame x 3, angle = 90 degrees (0x1a3500 x pi / 180 0x5a211c: sin 1, cos 0, so the offsets are not
  turned). Drawn as v0 v1 v2 / v1 v2 v3 in the sprite pass above.
- Argon: track_sprites.cpp (read, lists), sprite_pool.cpp (the bases), render\effect_buffers.cpp (EffectBuffers::writeTrackSprites), shaders\quadsprite.vert / .frag (texture x
  vertex colour, the fog; additive fogs to black), drawn after the trail sprites ("Track sprites" in the editor).
  Not ported: the shared pool order (the game sorts the spray / trail sprites and these in one list A; argon
  draws spray, trail sprites, then these), the alpha test (ref 0, no visible effect with blending), the ribbon trail
  drawn before the sprite pass.
