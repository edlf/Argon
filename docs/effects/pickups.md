# Effects: pickups

## Pickups (pickups_collide 0x8fcd0, weapon_pickup 0x6ba30)
Ported in `src\logic\pickups.cpp` (which pickup goes and comes back), `src\logic\particles.cpp` (the bursts) and
`src\app\app_pickups.cpp` (what a take gives: the levels, the points, the sounds).
- Table: 0x5293b0 + 0x1cf0 = 0x52b0a0, count 0x52afb4, 0x60 bytes each: +0 pos (pup.txt Y + per-type lift),
  +0xc type (0 blue, 1 red, 2 yellow, 3 mega-up, 4 power chip), +0x14 active, +0x18 time taken.
  Per-pickup "taken by the player this lap" flags at 0x52c860, cleared when a lap starts (0x63153) and at race start.
- The craft weapon update (0x6d000, every frame) calls pickups_collide(ship pos = craft +0x200, radius 14, time, &type,
  AI flag). Active pickup: box test |d| < 14 per axis, then |d| < 14; skipped if the player already took it this lap
  (players only) and power chips are skipped for AI / mode 4. Taken: active = 0, time stored, player flag set; sound
  0xc3470: types 0..2 SF_GENERIC_POWERPICKUP (audio\generic\silver), mega-up SF_GENERIC_MEGAPICKUP (gold), power chip
  SF_GENERIC_CHIPPICKUP (bronze). Inactive: back once time taken + 0.4 s has passed (power chips never), with a burst.
- Draw (pickups_draw 0x8fbc0, called twice per type from 0x93f1b): only active pickups; the normal model while the
  player hasn't taken it this lap, the `_ns` model (objects\powerup_<colour>_ns, megaup_ns) once taken. Matrix:
  identity rotation (0x50a180) at (x, y + 0.5 + 0.5 sin(2 now + index), z); no turn. The model draw (0xb83e0) sets
  the model's clock +0x5e4 to `now` (unless header flag 0x80000), so the tracks every pickup model has (powerup_red
  tetra1, powerup_blue sphere2, powerup_yellow cube1, megaup obj22, powerchip dodeca1 / obj1_1, the _ee / _ns ones)
  play on the race clock. Argon: SceneFrame::animatePickupModels (the frame clock).
- The power chip (type 4) changes no weapon: 0x6d000 only logs it (0x63d40 "POINTS BONUS" / "Picked up a points
  bonus", a debug log) and, as every pickup, records a replay event (0x64aa0 -> 0x649f0, kind 8) and plays its sound;
  its points are powerchip_bonus (0x83ea0, ported).
- weapon_pickup (types 0..3; craft weapon object): levels red +0xa4, blue +0xa8, yellow +0xac, special +0xb0.
  All three at 3: special + 1 (cap 0x52cd1e[player]). Otherwise red / blue / yellow + 1 (caps 0x52cd0c / 0x52cd12 /
  0x52cd18), mega-up all three + 1. Caps are set per race mode and upgrades at 0x3f4f1..0x3f6f6 (3/3/3 special 0 or 1,
  or 2/2/2). Then a burst on the ship.
- Burst 0x88750 (EAX = source position, flag, size, colour): 75 directions, two angles of (rand & 511) x 0.7045 deg,
  duration 0.5 + (rand & 127) / 255 s; per direction two rounds of three particles with velocity
  size x (cos a cos b, cos a sin b, sin a), x 1.1 after each particle and x 3 after each round, colour halved per
  particle and reset per round; colours red 0xffff3030, yellow 0xffffff30, blue 0xff3030ff, mega-up random of the three.
  Taken: flag 1 -> particle mode 3, size 10. Respawn (and replay event 9, 0x8ffa0): flag 0 -> mode 1, size 20.
- Particle systems (0x88660): 0x1baa28 data\hud\starburst.dds (1000 particles, point size 0.6, gravity -10, blend mode 2),
  0x1baa80 starburstred.dds (500, 0.5, -13), 0x1baad8 bubble.dds (1000, 0.35, 0). neon_add_particle 0xbcb60 entries
  (0x34): mode, pos, vel, source pointer, duration, born, delay, colour from, colour to. Update 0xbc640: mode bit 1 =
  implode (pos = source + vel x (1 - t)), bit 2 = source follows the pointer (the ship), else pos += vel dt and
  vel.y += gravity dt; colour lerps from -> to over the life. Draw 0xbc900: point list, point size x screen height /
  distance clamped 4..16 px, point sprites, no fog.
