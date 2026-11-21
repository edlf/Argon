# Effects: the water wake

## Water wake (emitter part of 0x50960)
Runs when `!0x52afac && craft+0x90 > 1 && !craft+0x94` (effects object = craft +0x850; +0x90 = how many of the rear
box corners 2 and 3 craft_water 0x61340 found under water, so both must be; +0x94 is set on every frame without
that and cleared on every frame with it, so the first frame in water only stores the position). Walks from last
frame's position (+0xd0, copied with the matrix +0x1d0.. into +0xa0..+0xdf each frame in water) to this frame's: when
the squared displacement is above 0.1 (0x1a3508), steps at s = 0, 2, 4 .. while s < its length (0x50d88 starts s
at 0, 0x51439 adds 2 (0x1a3534), so the first step is at last frame's position: at 60 Hz one step per side from
19 m/s, two from 432 km/h). Each sprite's height (0x50ed3 / 0x5101e): the water query 0xce6b0's out value is
added, not subtracted: puff y = (craft y - 4 forward.y -+ 2 right.y) + out - 1.5, droplet y = query y + out - 1.5,
out = query y - surface (or the query's own y when no box holds it, 0xce6b0 leaving it untouched). Constants: 0x1a3754 = 1/30, 0x1a3558 = 0.6,
0x1a35d4 = 0.85, 0x1a3684 = 1.2, 0x1a374c = -15, 0x1a3750 = 0.04, 0x1a36e0 = 15, 0x1a3688 = 1.5,
0x1a3564 = 255, 0x1a3444 = 4, 0x1a3554 = 5, 0x1a355c = 0.01, 0x1a3460 = 128.

## Splash on entering water: none in the code
No effect is spawned on the transition into water; what the game shows when a craft lands in water is this wake
(and the impact sound, [audio/race-sounds.md](../audio/race-sounds.md) "Splashes"). The search (2026-11-16):
- Readers of craft +0x92e / +0x92f (dispfind over every instruction): the physics (0x5dbf7..0x60e54: thrust,
  friction, upside-down reset, air time), the internal camera 0x5aa2b; none spawns anything. The effects object's
  +0xe4..+0xf8 (the rear corners' surface points, written by 0x61436..0x61474) are never read; its +0x90 only by
  0x50cf5 above.
- The effects object's vtable (one per ship, 0x1a0254..0x1a0404): [0] set-up, [1] 0x50950 (empty; the race scene's
  0x93823 calls it), [2] per frame (0x50960 + the ship's own animation), [3] 0x55a70, [4] 0x50840 (clears +0x90,
  sets +0x94).
- The sprite pool 0xbd670: 0x68311 (weapon sparks 0x68150), 0x687cf..0x696c9 (weapon beams 0x68620), 0x9d14e (trail
  sprites 0x9d000), 0x9d416 (droplets 0x9d2c0), 0x9d6f2 (puffs 0x9d620). 0x9d000's callers: projectiles, beams, the
  explosion 0x7de80, damage smoke 0x6cc80 and the hover's dust (0x5f4e6 / 0x5f662, "Dust" below). 0x9d620 /
  0x9d2c0: only the wake (0x50f8c / 0x512b0, 0x51100 / 0x51424: the two sides).
- neon_add_particle 0xbcb60: projectile trails 0x66952..0x669da, impacts 0x67435 / 0x68064, sparks 0x68273, pickup /
  respawn bursts 0x8888c, and the bubble system 0x1baad8 only in projectile trails of type 3 and the Atlantis / NMS
  camera bubbles (0x9420c / 0x99f8f). Explosions 0x7e0f0: weapons (0x660ef..0x6652a) and craft deaths (0x6a8ef /
  0x6a9de).
- data\hud\wavefront / wavefrontrear / wavefrontside / waverear / waverearside.dds (32 x 32 DXT5): no string in the
  XBE (ASCII or UTF-16) nor in any other file; the hud loader formats only tex%d / tex0%d (0x793be, pilot icons). They
  are boxed white icons of a wave pulse fired forwards, backwards and sideways: leftover weapon icons, not spray.
- The "Water Trail" / "WATERTRAIL" mesh effect (0x7db3a, ship draw 0x501c0) is no model's effect (no file names it).
Argon: the wake had skipped the step at s = 0 (no spray below 432 km/h, half above) and started on any corner or
hover ray over water; now as above (logic\water_wake.hpp, WakeSpray::follow / emitStep in effects\wake_spray.cpp, GameCraft::wakeCorners).

## Spray puffs (add 0x9d620, update 0x9c540)
- Pool 0x240540, 4096 entries of 0x50, bitset 0x1f0330. Fields +0x1f8 pos, +0x204 vel, +0x210 W, +0x214 H,
  +0x21c born, +0x220 life, +0x224 fade = life x 0.85, +0x22c max height, +0x238 / +0x23c colours,
  mode +0x242 = 1: height = sin(age / life x pi) x max height.
- Per step and side: at the step point - 5 x forward -+ 2 x right, +-2.55 m random in x / z, 1.5 m below.
  Life 0.5..1.53 s, alpha 16..79 to 0 over the last 15 %, half-width (2..3.03) x 0.6 with a random mirror,
  height (4..7.9) x 0.6.
- Sprite via 0xbd670 (EAX 1: upright, the view's x axis flattened and y up, so the puffs stand on the water
  whatever the camera's pitch; blended pool, spray flag 1), quadsprite.dds atlas cell (0, 384), 128 px; vertex
  shader quadsprite.xvu. Mode 1 offsets (0x9c540): v0 (-W, H), v1 (W, H), v2 (-W, 0), v3 (W, 0), not turned (spin
  and start angle 0). Argon: EffectBuffers::writeSpray (WakeSpray::writePuffs) with sprite_pool.hpp (it had faced them fully to the camera).

## Droplet sheets (add 0x9d2c0, update inline in 0x9ca50)
- Second pool: bitset 0x240538, 4096 entries of 0x50 at 0x1f0338 + 0x200: +0 pos, +0xc side vector,
  +0x18 vel, +0x24 streak length, +0x28 gravity, +0x2c born, +0x30 life, +0x34 hold time, +0x38 / +0x40
  start colours, +0x3c / +0x44 end colours, +0x48 sprite list, +0x49 fade-in flag, +0x4a static, +0x4c sprite.
- Args (EAX = atlas cell, EBX = side vector): pos, vel, life, start colour, end colour, hold, streak,
  gravity, list, fade-in, param 11.
- Wake call (50 % per step and side): pos = step point - 4 x forward -+ 1.2 x right, 1.5 m down;
  vel = displacement since the last update -+ 15 x right, y + (64..95) x 0.04; side = 4 x forward;
  life 0.5, colour alpha 32..63 white -> 0x00ffffff, hold 0.2, streak 8, gravity -10, list 0 (alpha),
  cell 13 = (128, 384).
- Update: pos += dt x vel, vel.y += dt x gravity. Corners: pos -+ side, pos + streak -+ side, with
  streak = normalize(vel) x length. Colour: start colour until the hold time (or a fade-in from 0 if +0x49),
  then a linear blend to the end colour by the end of life.
- The quad: 0xbd670 with EAX 0 (world offsets), size 1, the wake's list 0 = the blended pool, spray flag = param 11;
  0x9d2c0 then writes the angle slots (cos 0, -sin 0) = (1, 0) (its random alloc angle is never used) and v0 / v1 =
  -side / +side; 0x9ca50 writes v2 / v3 = streak -+ side, the base's colour (+0x38 -> +0x3c) to v0 / v1 and the
  head's (+0x40 -> +0x44) to v2 / v3 (0x9d2c0 gives both ends the same pair). So v0 (low u, low v) is at the base, v2 (low u, high v) at the head; argon
  had the cell upside down.
