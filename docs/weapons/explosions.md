# Weapons: explosions and knock-outs

## Explosions (impact 0x65f80 -> 0x7e0f0, freed 0x7de10, drawn and set off 0x7de80; models 0x7dd40)
- The impact runs bursts A and B (+0xd4): `count` explosions of `type` each `delay` s apart; a burst runs on impact
  kind 1 / 2 with flag 1, 3 with flag 2, 4 with flag 8. Explosion k of a burst lands at the impact + ((rand & 0x3ff) -
  512) / 512 x the burst's scatter per axis (A +0xe4, 0 in the data; B +0xe8, 10..50), y 0 when burst A's type has
  +0x20 (both bursts read A's type there, 0x6601f / 0x66310); it starts k x delay after the impact; it is asked for a
  duration and a size both size % x 0.01 (+0xda / +0xdb); it carries the weapon's effects (record +0x4c) unless flag
  0x10. Then, if any burst ran (a count above 0), a flash per burst with flag 4 (0x6659d): 0x9d000 cell 15, still,
  white to transparent black over 0.5 s, growth 0 (it shrinks), no spin, additive, half size the area inner radius
  (+0x80), 10 when that is below 10. The projectile update adds its sparks around it (0x67231: a craft hit, 250
  starburst, a random unit direction x 3 m/s, 0.1 m out, life 3 + (rand & 0xff) / 255 x 0.5; 0x67e81: every impact but
  a bounce, after the special's impact sound, 50 starburstred, a random direction x (10 + (rand & 0xff) / 255 x 20)
  m/s with y made positive, 0.1 x the velocity out, life 1 + (rand & 0xff) / 255 x 0.25; both hold their white for 0.8
  of the life, then blend to transparent black).
- The explosion types: weapon.nwp's last 0x1480 bytes (0x1e8bb8, 32 of 0xa4; only 0..15 have data and a model):
  +0 clock factor, +4 duration factor, +8 size factor (%), +0xc flash factor, +0x10 / +0x14 not read, +0x18 push
  (%), +0x1c passed to 0x7e410 and not read, +0x20 flat scatter, +0x21 tumbles (random yaw, pitch and roll), +0x22
  turns (random yaw only; it wins over +0x21). Data:

  | type | clock | duration | size | flash | push | flags | model (objects\explosion_type<n>) |
  |---|---|---|---|---|---|---|---|
  | 0 | 1 | 2 | 200 | 500 | 100 | - | 8 meshes: spheres, cylinders, a grid |
  | 1 | 1 | 1 | 50 | 50 | 100 | tumbles | grids, revol13 |
  | 2 | 0.8 | 3 | 100 | 50 | 100 | tumbles | sphere1, revol13 |
  | 3 | 1 | 1 | 100 | 50 | 100 | tumbles | sphere1, obj6 |
  | 4 | 1 | 1 | 50 | 2000 | 100 | - | sphere1, obj2, obj3 |
  | 5 | 1 | 1 | 300 | 200 | 100 | flat | grid15, obj1, obj3 |
  | 6 | 1 | 1 | 100 | 50 | 100 | tumbles | grid1 x 3 |
  | 7 | 1 | 6 | 150 | 800 | 100 | - | revol1..4, torus6 (the NUKE's) |
  | 8 | 3 | 2 | 200 | 20 | 100 | flat, turns | obj1 x 2 |
  | 9 | 1 | 5 | 500 | 200 | 100 | - | grid1..4 (camera-facing, [effects/README.md](../effects/README.md)) |
  | 10 | 2.7 | 3 | 100 | 20 | 100 | - | revol1_1 x 2 |
  | 11..13 | 0 | 0 | 0 | 0 | 0 | - | cube1: never seen (duration and scale 0) |
  | 14 | 1.8 | 1 | 200 | 200 | 0 | flat | RADIAL_BLUE0, revol10..12, sphere_1 |
  | 15 | 1 | 1 | 300 | 200 | 100 | - | grid1 (one quad, no texture) |

- 0x7e0f0(type, duration, start, size, follow, ...) at EDI = position: a type past 15 is 15; the duration x the type's
  duration factor, the scale = the type's size factor x 0.01 x size; it takes the first free of the 64 slots (duration
  0) and the first free of its model's 16 copies (0x1ea478), else nothing is made at all (no flash, push, damage or
  sound: a crowded impact loses explosions, so 8 projectiles hitting at once can lose their later ones). Three rand()
  & 0x3ff x 1 / 1023 x 360 degrees: the second the yaw, the first the pitch, the third the roll
  (D3DXMatrixRotationYawPitchRoll 0xf3117), kept by the type's flags; the matrix's rows x the scale, the position the
  impact's. Its other arguments (follow a point, a count of sub-points and their spread, the last one) are stored and
  never read.
- 0x7de10 (each frame, the game clock 0x50e430): an explosion is freed (slot and copy) once start + duration < now.
  0x7de80 (each view's frame, after the crafts): every explosion with start <= now <= start + duration is drawn: every
  bone's clock (+0x188, own clocks: 0x7dd40 set flag 0x80 on every bone of every copy) = progress x the clock factor,
  progress = (now - start) / duration, so its tracks play once over its life (more with a factor above 1); every
  bone's tint (+0x104, 0xb99b0; lit meshes only) the grey g x 255 >> 8, g = clamp(255 - fistp(255 x progress)), all
  four bytes; drawn through the render list (0xb83e0 argument 1: camera-facing meshes face the camera). The first such
  frame it goes off (+0x4c): a flash when the flash factor is above 0 (0x7df35: the impact flash's sprite with half
  size scale x flash factor), then, carrying effects: the push (0x7e410: x the type's +0x18 %; the next physics step
  0x5dee0 turns it into the rumble's knock, a speed loss, a sideways shove and the cockpit camera's shake: [physics/README.md](../physics/README.md)
  "Rumble"), for an area weapon (flags 0x400) the hit on every craft (0x64ae0 -> 0x6acf0 from the explosion, the area
  factor picking who it reaches) and the projectile impact sound twice (0xc3050). An explosion carrying nothing (burst
  flag 0x10, a knock-out's) makes no sound. No light is set: the models' lit meshes take the scene's light. So
  CLUSTER level 1 (both bursts 0x10) never damages; NUKE (burst A, 1 explosion) hits once.
- Argon: logic\explosion.hpp (the types, sizes, scatter, turn, matrix, life, grey, flashes, the 64 / 16 room, the
  knock-out's, the sparks; tests\explosion_tests.cpp), logic\explosions.cpp (Explosions: starting them, their slots,
  which go off), weapon_explosions.cpp (Weapons::startImpactExplosions, updateExplosions, goOff: in the 60 Hz step at the simulation clock, not in the draw), SceneRecorder::drawExplosions
  (the model at its matrix, the bones posed at their clock by Cmm::pose, the grey as the bone tint, both passes, every
  view). The models load when their first explosion starts (the game loads all 16 at start-up). A replay's impacts
  start them too (no push; their damage is in the shield entries). Before, argon drew no explosion at all (burst
  particles stood in), made every explosion play the impact sound and put every explosion of a burst on the impact.

## Knock-out (0x6a8a0 -> craftdeath 0x61fc0, 0x58d20)
- Explosions of type 6 carrying nothing (0x6a8c3): one at the craft (duration 5, size 3: 5 s at scale 3), then
  three (duration 2, size 1) the k-th 0.2 k s later at the craft + ((rand & 0x7ff) - 0x400) / 1024 x 20 m per axis. craftdeath: the killer (0x1c5050 + id x 4), the
  time (0x1c4ff8), racer slot +0x6b; 0x58d20 sets craft +0x1145 (out: no weapons, no hits, no AI pickups, no thrust)
  and, if not finished, racer +0x6a and 0x638b0 (DNF, the finish order filled from the back). A human player's HUD
  gets a message (object vtable 0x1a1d68 to "hud<n>": "set none").
- The cage (0x61f20, race setup on the track model 0x52cddc + 0x1704): the bone NET_CAGE's matrix is kept at
  0x1c5010 (its position 0x1c5040), NET_GLOW's bone index at 0x1c4ff4, hidden (0xb97f0(bone, 0): bone flags +0x102
  bit 0). craftdeath shows NET_GLOW (0xb97f0(bone, 1)) and 0x58d20 puts the craft into both state blocks at the
  cage's position, rotation identity, all velocities and momenta 0 ("Emergency Teleport"). Every track has the bones.
- The player (0x620e0, each frame for a racer with +0x6b; human slots only): the camera's eye = cage position + the
  cage's rotation x (20 - 30 f, 100 - 150 f, 120 - 110 f), f = 1 - (1 - t / 6)^3 capped at 1 (t = now - the death
  time), looking at the craft (+0x200), up (0, 1, 0), fov pi x 0.4. Single player: once t > 10 or A is down (pad bit
  0x10) while racing (0x52afa8 = 0): the levels and cash are kept for Race Again (0x1bc87c.., flag 0x1bc88c) and
  LOSEMENU opens directly (0x52afa8 = 2: no POSTABLE, SCOREMENU or tournament progress 0x95d80). Split screen: when
  at most one human still races, the last one is put in the finish order and MULTIPLAYERENDMENU opens (state 7).
- Messages (0x624f0, from the viewport draw 0x93d97; the player's viewport x0 +0x2a0, y0 +0x2a4, w +0x2a8, h +0x2ac;
  font 1 in single player, else 0): while racing and not 0x52af9f, for 6 s "DAMAGE CRITICAL" (race 0x51d) red
  0xffff0000 at (centre, y0 + h / 4) and "Emergency Teleport" (0x51e) yellow 0xffffff00 at y0 + 3h / 4 (flags 2,
  centred); always "You were destroyed by" (0x514) and the killer's name (0x78b00) as "%s\n%s" (0x1a0200), white at
  y0 + h / 2 (flags 0x12). Once a menu is up the killer is cleared (-1) and nothing is drawn.
- "You Died" (race 0x40d) is not referenced by any immediate in the code (LOSEMENU's title is "You Lost" 0x3e8): it
  may be unused.
