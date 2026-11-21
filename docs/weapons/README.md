# Weapons (craftweapon.cpp, from default.xbe)

Shared core, decoded so far. Ported in `src\logic\weapon_book.cpp` (weapon.nwp's records, resistances and explosion
types), `src\app\weapons.cpp` (firing, volleys, lock-on), `src\app\weapon_shots.cpp`
(projectiles and beams; their rules `src\logic\shot_flight.cpp`, tests\shot_flight_tests.cpp; their frames
`src\logic\shot_motion.cpp`, tests\shot_motion_tests.cpp),
`src\logic\explosions.cpp` (the explosions in flight; tests\weapons_tests.cpp) with `src\app\weapon_explosions.cpp`
(what going off does) / `src\logic\explosion.cpp` (impacts, explosions, sparks; tests\explosion_tests.cpp),
`src\app\weapon_hits.cpp` (hits, shields, area blasts, knock-outs) with `src\logic\craft_weapon.cpp` (what they
do to one craft's weapon object; tests\craft_weapon_tests.cpp), `src\logic\ai_trigger.cpp` with
`src\app\weapon_ai.cpp` (the AI's trigger: the rules, and what App gathers and fires; tests\lock_on_tests.cpp), `src\effects\weapon_sprites.cpp` with `src\app\app_trail_sprites.cpp` (trail sprites and the
flashes), `src\logic\weapon_damage.cpp` (the hit arithmetic;
tests\weapon_damage_tests.cpp) and `src\logic\weapon_rules.cpp` / `trail_sprite.cpp` (drones, nemesis race, shields'
fade and sphere, trail sprites; tests\weapon_rules_tests.cpp), `src\logic\weapon_flash.cpp` (the beams' flashes;
tests\weapon_flash_tests.cpp).

| file | what |
|---|---|
| [ai.md](ai.md) | the AI's firing and the nemesis |
| [argon.md](argon.md) | argon's port |
| [explosions.md](explosions.md) | explosions and knock-outs |
| [firing.md](firing.md) | firing and projectiles |
| [hud.md](hud.md) | flashes, lock-on and warnings |
| [shields-beams.md](shields-beams.md) | shields, volleys and beams |

## data\weapon.nwp (loader 0x640c0)
- 16 pilot blocks of 0x2384 bytes, indexed by pilot id (Tara 0 .. Kutoo 15), copied to 0x1c5270; then 0x1480 bytes to
  0x1e8bb8 (not decoded). The craft weapon object (craft +0x854) keeps +0x90 = its pilot's block, +0x44 = the craft.
- Block: u32 count (11), the pilot's 17 resistances (effect channels below, %), then 11 records of 0x334 bytes at block + 0x48 + i x 0x334. Record i = slot:
  slots 0..8 = (level - 1) x 3 + colour (0 red, 1 yellow, 2 blue), 9 special, 10 retaliator.
  Tara: ROCKET / TSHIELD / CLUSTER x 3 levels, NUKE, RETALIATOR; others: S1FANGS / TSHIELD / LASER, ...
- Record fields (offsets from the record):

| off | type | what |
|---|---|---|
| +0x08 | i32 | class: 0 projectile, 1 beam (LASER, life 0.2), 2 shield (TSHIELD) |
| +0x0c | char[32] | name (= the ship model's bone for its mesh) |
| +0x4b | u8 | use +0x190 as the hit radius (else 4) |
| +0x4c | 17 x f32 | damage: the effect channels below |
| +0x7c | u32 | (damage channel 12) flags: 0x400 = area weapon, no direct hit damage (CLUSTER, NUKE, FIREBOMB, STAR) |
| +0x90 | 17 x f32 | shields: added to the owner's resistances while up (0x6aa00: channels 0, 1, 2, 4, 5, 7, 9) |
| +0xd4 | 16 bytes | impact bursts A / B: explosion type, flags, count, size %, delay between explosions (s); flags 1 on hit kind 1 / 2, 2 on hit kind 3, 8 on hit kind 4, 4 an impact flash, 0x10 the explosions carry nothing (else the weapon's effects) ("Explosions") |
| +0xe4 / +0xe8 | f32 | bursts A / B: each explosion lands up to this far off the impact per axis (A 0 in every file; B 0..50) |
| +0xec | i32 | trail type: 1 starburst, 2 starburstred, 3 bubble, > 3 sprite |
| +0xf4 / +0xf8 | ARGB | trail colour from / to |
| +0xfc | f32 | trail spacing (m) |
| +0x100 | f32 | spawn / trail offset back along the projectile's forward |
| +0x104 | f32 | trail particle speed back along the forward |
| +0x108 | f32 | trail jitter |
| +0xf0 | u32 | sprite trails: 1 random spin direction, 2 alpha blended (list A, else additive list B), 4 fade in over +0x124 |
| +0x110 / +0x114 | f32 | sprite trails: half size + (rand & 0x3ff) / 1023 x size range |
| +0x118 | f32 | sprite trails: growth, the size x (1 + (+0x118 - 1) x 20 dt^2) a frame |
| +0x11c / +0x120 | f32 | sprite trails: spin (degrees / s) + (rand & 0x3ff) / 1023 x spin range |
| +0x124 | f32 | sprite trails: the colour holds (or fades in) this long, then blends to +0xf8 (0 in the data) |
| +0x128 | f32 | trail particle life (s) |
| +0x13c | u8 | repeats: each projectile relaunches every count x stagger while +0x144 lasts (beams, MINIGUN, 1PULSE) |
| +0x144 | f32 | repeat time (s after the shot) |
| +0x13e | u8 | homing |
| +0x13f / +0x18c | u8 / f32 | launched along the craft's own axes (not the bone's), nose up by +0x18c degrees (0..90) (0x65b0e) |
| +0x140 | f32 | life (s) |
| +0x148 | f32 | above this speed (or always with bounces) the frame turns along the path each step (0x67add) |
| +0x1b0 / +0x1b3 | u8 | shields while up: flags 4 absorb (a hit's shield damage recharges the shield, 0x6ab60), 0x10 retaliate (a hit fires slot +0x1b3, 10 = RETALIATOR, at the attacker, 0x6abf0) |
| +0x150 | u8 | lock-on list (targets from the weapon object +0x58 / +0x60) |
| +0x152 / +0x155 / +0x154 | u8 | hug the surface below / above / to the sides at +0x15c; to the sides also slides along walls (0x674e7) |
| +0x156 | u8 | with bounces: no more acceleration once bounced (0x66f4b) |
| +0x15c | f32 | hug distance |
| +0x160 | f32 | homing turn rate (x 60 x dt %, max 100 %) |
| +0x165 | u8 | bounces |
| +0x167 | u8 | with +0x150 and a target: launched looking at it (0x65a7d; Abi's SPECIAL, every RETALIATOR) |
| +0x168 | i32 | projectiles per shot |
| +0x170 | f32 | stagger: projectile k leaves k x +0x170 s after the shot (0x6a390 / 0x69e30) |
| +0x16c | i32 | max bounces |
| +0x174 / +0x178 | f32 | acceleration / max speed |
| +0x17c | f32 | drop: fall speed grows by this per second |
| +0x180 | f32 | % of the craft velocity inherited |
| +0x188 | f32 | homing lock range |
| +0x190 | f32 | hit radius (with +0x4b) |
| +0x194 | f32 / i32 | projectiles: homing delay (s, 0x66feb); shields (class 2): read as an int, the bone mode at launch (0x653c3: 0 attached, 1 / 3 world-aligned, 2 / 3 detached; [models/meshes.md](../models/meshes.md) "Bone flags at run time"). Data: 0 in every record but Darius's and Jaya's SPECIAL (0.5, projectiles), so every shield is attached |
| +0x198 / +0x19c | f32 | start speed = craft forward speed x +0x198 % + +0x19c |
| +0x1a0 / +0x1a4 | f32 | shields (0x69970): the model's colour goes from +0x1a8 at age +0x1a0 to +0x1ac at age +0x1a4 (TSHIELD: 2.7 -> 3 s of its 3) |
| +0x1a8 / +0x1ac | ARGB | shields: colour from / to |
| +0x184..+0x190 | f32 | shields (0x69970): between ages +0x184 and +0x188 a sphere from the shield bone grows from radius +0x18c to +0x190; each other craft closer than it gets impact kind 1 once (+0x16d0) (TSHIELD 0..4 s, 1 / 2TSHIELD 0..2 s, 2 -> 100 m; their bursts are empty, so nothing happens) |

## Effect channels (damage 0x6acf0, resistances 0x6aa00)
17 floats, in the damage block, the shield bonus and the pilot's resistances: 0 shield damage (% of the shield),
1 shield leech (the shooter gets this % of the shield damage back, 0x6b7e0), 2 / 3 corrode level (shield per second)
/ time, 4 freeze time, 5 / 6 control jitter / time, 7 / 8 engine jitter level / time, 9 engine-off time, 10 / 11
unused, 12 flags, 13 / 14 area inner / outer radius, 15 / 16 area inner / outer %. Area weapons: inner % within the
inner radius, falling linearly to the outer % at the outer radius, nothing beyond. Each channel is scaled by that
and by (100 - the target's resistance) %; corrode level uses the shield damage resistance, the times their level's.
The pilot's shield resistance is combined with a per-pilot byte (0x52cd24); active shields (class 2) add their
+0x90 values. craft_weapon_update (0x6d000) counts the times down (CORRODE DONE, ...).

## Effects on the craft (craft_weapon_update 0x6d000, craft update 0x5b1d4, thrust 0x5e370, steering 0x5f9d0)
- The weapon object (craft +0x854) keeps +0x8 / +0xc corrode level / time, +0x10 freeze time, +0x14 / +0x18 control
  jitter level / time, +0x1c / +0x20 engine jitter level / time, +0x24 engine-off time; a hit makes each the larger of
  its value and the hit's (0x6b1b8; not when a shield absorbs; engine off plays the engine-off sound 0xc2ef0).
- 0x6d000 each frame, unless the craft is out (+0x1145): every time counts down by dt to 0; a jitter's level goes to 0
  with its time; engine off reaching 0 plays the engine-on sound (0xc2f40); corrode reaching 0 zeroes its level
  ("CORRODE DONE"), else, unless 0x52af9f, a shield above 0 loses level x dt % (halved for craft 1 in a nemesis race,
  0x791f0), a 0.2 flash (0x6d400, "Flashes" below), stopping at 0 % ("CORRODE ZERO"): corrode never knocks out.
- The craft update copies the times to the craft each frame (0x5b1d4, unless 0x52afac): +0xa00 engine off, +0xa04 /
  +0xa08 control jitter level / time, +0xa0c / +0xa10 engine jitter level / time, +0xa14 freeze.
- Thrust (0x5e475): freeze or engine off > 0: the throttle +0x8f4 = 0; engine jitter time > 0: the throttle loses
  (rand & 15) x level / 15, at least 0. Both write the throttle back (the HUD's needle shows it).
- Steering (0x5fa79): control jitter time > 0: each smoothed steering input (+0x974, then +0x970) loses (rand & 15) x
  level / 15, at least 0. So jitter weakens the throttle / steering at random each frame; freeze is the same as engine
  off for the physics (no other reader of +0xa14 / the weapon object's +0x10 found; any look of it is not decoded).
- A shield with a shield bonus (class 2, +0x90 > 0) clears every effect at its launch (0x6a4df, after the recharge;
  the engine jitter level stays, its time goes); the launch 0x64e20 also scales the running ones by (100 - the
  bonus) % (0x65766), which the clearing makes moot.
- Data: ROCKET engine off 1 s, LGUN control jitter 10 for 2 s, PULSE engine off 0.2 s (seen in argon runs).

## Hits (weapon_damage 0x6acf0: eax = where, args target weapon object, effects, now, shooter, flag)
Read with the disassembler (the decompiler times out); the arithmetic is `src\logic\weapon_damage.cpp`.
- No effect on the shooter itself. Records the target's last-hit time (racer slot +0x70).
- Area factor (flags 0x400 only, else 1): d = |target craft +0x200 - where|; t = max(d - inner radius, 0) / (outer -
  inner radius) (outer <= inner: nothing); t >= 1: nothing; factor = (inner % - outer %) x 0.01 x (1 - t) + outer % x
  0.01. A positive factor plays a sound at `where` for a human target.
- Resistances (0x6aa00): the pilot's 17, but channel 0 = 100 - 10000 / shield size (shield size 0x64030 = 20 x level
  + 100 / ((100 - pilot resistance 0) x 0.01)), so a hit takes `damage` points of any shield; then every active class 2
  slot adds its +0x90 to channels 0 (capped at 100), 1, 2, 4, 5, 7, 9.
- out k = factor x damage k x (100 - res) x 0.01 with res: 0 shield damage res 0, 1 leech res 1, 2 corrode level res 0,
  3 corrode time none, 4 freeze res 4, 5 / 6 control jitter res 5, 7 / 8 engine jitter res 7, 9 engine off res 9.
- Absorb (0x6ab60: an active shield with flags 4): out 0 = -out 0 (a recharge). Else retaliate (0x6abf0: an active
  shield with flags 0x10 gives slot +0x1b3; 0x6a6c0 launches it, every projectile aimed at the attacker, unless that
  slot is already out). Blocked (0x6aba0: an active shield with +0x90 channel 0 > 0): every out k = 0 except a
  negative out 0.
- Leech (0x6b7e0): unless the shooter is knocked out, the shooter's shield gains out 0 % of the target's shield size
  x out 1 % (capped at its full shield).
- The Infinite Shield cheat (0x1bbbf0) stops here for the player. Nemesis race (0x791f0): a hit on craft 1 (the
  nemesis) is halved.
- Unless 0x52af9f: energy (weapon object +0, in % of the shield size) -= out 0, capped at 100. Stats: the player's
  damage dealt (0x83f90, doubled on the nemesis), taken (0x84010). A hit with out 0 > 0 flashes the target
  (0x6d430 human / 0x6d400 AI, 0.8).
- Unless absorbing, the timers take the larger of theirs and the hit's: +0x14 / +0x18 control jitter, +0x10 freeze,
  +0x1c / +0x20 engine jitter, +0x8 / +0xc corrode, +0x24 engine off (with a sound).
- Energy <= 0: in a one-player race, the player hit from 33 % or more is left at 1 + (rand & 0x3ff) x 5 / 1023 %.
  Still <= 0 and not already out: knock-out (0x6a8a0) and the kill stat (0x83e00).
