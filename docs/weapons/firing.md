# Weapons: firing and projectiles

## Fire buttons (player_pad_input 0x70ac0, player controller 0x718xx)
- 0x70fd2, every frame: the pad's held bits +0x88 (0xb4800: A 0x10, B 0x20, X 0x40, Y 0x80) -> craft +0x91c = B,
  +0x91d = X, +0x91e = Y (cleared at 0x712ef / 0x7139c).
- 0x71918: with Weapons on (0x52abe3) and the race running (0x52af9f clear) one request a frame to 0x6d000: +0x91c
  colour 1 (red), else +0x91d colour 3 (blue), else +0x91e colour 2 (yellow), else 0 (no fire, the update still runs).
  Held, not pressed: a held button fires as soon as its colour is armed and its last shot is gone (0x6c110 blocks
  while it is out, the level empties after a shot). No special button: a held special takes any request (0x6d000).

## Firing (weapon_select 0x6c110, from craft_weapon_update 0x6d000)
- 0x6d000: while the craft holds a special (+0xb0) any fire request (not 0) becomes colour 4 (0x6d023); after it the
  yellow level is set to 3 (+0xac) and colour 2 fires too (0x6d0b1), whether or not the special left. Neither happens
  while the craft is out (+0x1145, 0x6d030), the race is over (0x52af9f) or the object has no pilot block (+0x90). The
  drone (0x6e9e0) asks with colours 1 / 3 / 2, so it fires its special this way. So a special is always followed by
  the level 3 shield (2TSHIELD: up 7 s, blocks every hit, retaliates with RETALIATOR, recharges 30 points), unless a
  yellow shield is still up (then the yellow level stays 3 for the next request).
- Colour 1 red / 2 yellow / 3 blue / 4 special: slot = level x 3 - 3 / - 2 / - 1, 9; nothing below level 1.
  Blocked while that colour's previous shot is alive (red: slots 0 / 3 / 6, +0xc0 / +0x4650 / +0x8be0; yellow
  0x6c0b0; blue 0x6c0e0; special slot 9, +0xd170).
- Records the time (+0xffd0) and the targets (0x6c22e..0x6c2f2): every projectile's target (slot object +0x370 + k x 4)
  is cleared to -1; a lock-on weapon (+0x150) with locks (+0x58, the list +0x60 nearest first, 0x6c420 this frame)
  first aims every projectile at the nearest lock, then projectile k < locks at the k-th. So an 8-projectile special
  with 2 locks sends 1 + 6 at the nearest and 1 at the second. Then it launches every projectile (0x6a390 -> 0x64e20),
  then that colour's level and the special count go back to 0; the special (slot 9) clears all three levels
  (0x6c33e: slots 0 / 3 / 6 / 9 red, 1 / 4 / 7 / 9 yellow, 2 / 5 / 8 / 9 blue, +0xb0 always).
- The special's grant (weapon_pickup 0x6ba30, the pickup type in the argument: 1 red +0xa4, 0 blue +0xa8, 2 yellow
  +0xac, 3 mega-up all three): with all three at 3 any pickup adds a special (+0xb0, capped by the craft's 0x52cd1e:
  the player's Weapon Strength 7, a drone's speed class and kind), else the type's levels rise by 1 to their caps
  (0x52cd0c / 12 / 18). It also moves +0x9c (+1, mega-up +5, at most the record count - 1) for the replay's pickup
  entry (0x64970 kind 6) and refreshes every player's shield flash (0x6b950).
- The specials (slot 9) are area weapons (flags 0x400): no direct hit, each projectile's burst A (flags 7: on a craft,
  the world or expiry) sets off an explosion carrying the damage, inner 10 m 100 % to outer 45 m 60 %. Shield damage
  per explosion: NUKE / DATABOMB / SHARD / TERRABALL / SNOVA / RAILBLAST / NAPALM / STRIKE 200 (one projectile),
  SPECIAL / MEGA 25 (Marcus 33.3) x 8, LSTRIKE 67 x 3, 1PHANTASM 50 x 4; all lock on. Per pilot (weapon.nwp record
  9, the firing ship's own; Weapons::pilotWeapon): Tara NUKE, Yuri SPECIAL, Kumiko DATABOMB, Darius SPECIAL, Jordan MEGA,
  Marcus SPECIAL (33.3), Leah SHARD, Ka Hekili TERRABALL, Alicia LSTRIKE, Vladimir 1PHANTASM, Abi SPECIAL, Zafir
  SNOVA, Kaitlyn RAILBLAST, Jaya SPECIAL, Lukas NAPALM, Kutoo STRIKE. Checked on a real Xbox (2026-11-16): a special
  fires its damaging weapon and the shield, and the red / blue / yellow levels drain to 0.
- What each special looks like (the record's data through the rules above; "Explosions", "Projectile", "Trail
  sprites"). Every projectile is the bone "<name><k>" of the firing ship, hugs the ground below it at +0x15c and locks
  on; it leaves at the craft's forward speed + 100 m/s (Ka Hekili + 50) and speeds up to the maximum. Trail: a sprite
  in quadsprite.dds cell n with its start colour (ARGB). A: burst A's one explosion, the damaging one (type, life x
  scale, its flash's half size in m). B: burst B's, carrying nothing (count x type, spacing, scatter per axis).
  Impact flash: the bursts with flag 4 (half size 10 m each).

  | pilot | record | projectiles | trail | A | B | impact flash |
  |---|---|---|---|---|---|---|
  | Tara | NUKE | 1, to 1200 m/s, 3 s, nose up 2 deg | cell 0 80ffffff | type 7, 6 s x 1.5, flash 1200 | 10 x type 6 (2 s x 2, flash 100) 0.2 s apart, 50 m | A, B |
  | Yuri | SPECIAL | 8 at once, to 1000 m/s, 3 s | cell 8 406fff88 | type 6, 1 s x 1, flash 50 | 2 x type 15 (0.12 s x 0.36, flash 72) 0.3 s apart | A, B |
  | Kumiko | DATABOMB | 1, to 1200 m/s, 3 s, nose up 4 deg | cell 0 7dd0ccb0 | type 4, 2 s x 1, flash 2000 | 2 x type 3 (2.5 s x 2.5, flash 125) 0.2 s apart, 20 m | A, B |
  | Darius | SPECIAL | 8, 0.05 s apart, to 1000 m/s, 3 s, homes after 0.5 s | cell 3 407d420d | type 6, 0.75 s x 0.75, flash 37.5 | 2 x type 15 (0.12 s x 0.36, flash 72) 0.3 s apart | A, B |
  | Jordan | MEGA | 8, 0.2 s apart, to 2000 m/s, 3 s | cell 3 ff31326c | type 3, 1 s x 1, flash 50 | 3 x type 5 (0.1 s x 0.3, flash 60) 0.25 s apart, 10 m | A |
  | Marcus | SPECIAL | 8, 0.1 s apart, to 1000 m/s, 3 s, 1 bounce | cell 1 401c585e | type 3, 2 s x 2, flash 100 | 2 x type 3 (1 s x 1, flash 50) 0.3 s apart, 20 m | A, B |
  | Leah | SHARD | 1, to 1200 m/s, 4 s, nose up 2 deg | cell 3 103f6165 | type 14, 2 s x 4, flash 800 | 12 x type 12: nothing to see | A, B |
  | Ka Hekili | TERRABALL | 1, to 600 m/s, 4 s, nose up 4 deg | cell 3 ffa35703 | type 2, 2.4 s x 0.8, flash 40 | 10 x type 6 (1 s x 1, flash 50) 0.1 s apart, 30 m | A, B |
  | Alicia | LSTRIKE | 3, 0.2 s apart, to 1540 m/s, 3 s, nose up 2 deg | cell 6 ff484ddf | type 5, 1 s x 3, flash 600 | 5 x type 3 (1 s x 1, flash 50) 0.2 s apart, 20 m | - |
  | Vladimir | 1PHANTASM | 4, 0.5 s apart, again for 2 s, to 1800 m/s, 3 s, nose up 1 deg | cell 11 c0ff5959 | type 2, 1.5 s x 0.5, flash 25 | 10 x type 15 (0.1 s x 0.3, flash 60) 0.1 s apart, 10 m | - |
  | Abi | SPECIAL | 8, 0.15 s apart, to 1000 m/s, 3 s, 1 bounce, aimed at its target | cell 4 4062c8d0 | type 3, 1 s x 1, flash 50 | 1 x type 15 (0.05 s x 0.15, flash 30) | - |
  | Zafir | SNOVA | 1, to 1500 m/s, 3.5 s, nose up 1 deg | cell 10 ffd6c712 | type 9, 2.5 s x 2.5, flash 500 | 5 x type 9 (0.5 s x 0.5, flash 100) 0.2 s apart, 30 m | A, B |
  | Kaitlyn | RAILBLAST | 1, to 7000 m/s, 3 s | cell 15 ffff090f | type 3, 1.5 s x 1.5, flash 75 | 1 x type 6 (1 s x 1, flash 50), 20 m | A, B |
  | Jaya | SPECIAL | 8 at once, to 400 m/s, 3 s, homes after 0.5 s, hugs walls | cell 13 c0ffffff | type 10, 3 s x 1, flash 20 | none | B |
  | Lukas | NAPALM | 1, to 1000 m/s, 3 s | cell 10 fff1bf87 | type 2, 6 s x 2, flash 100 | 10 x type 9 (0.5 s x 0.5, flash 100) 0.2 s apart, 50 m | A, B |
  | Kutoo | STRIKE | 1, to 1200 m/s, 3 s | cell 1 ff2f38aa | type 0, 4 s x 4, flash 2000 | 5 x type 15 (0.2 s x 0.6, flash 120) 0.2 s apart, 25 m | A, B |

  Seen in argon (`--pilot <p> --quick-race 1 --race-go --ai-race --autofire --special-at 10 --nopanel
  --drive-time 10.25` / `10.8 --shot <png>`, every pilot): each special's explosions drawn as above, the near ones'
  flashes (Yuri, Kumiko, Alicia, Abi, Kaitlyn, Jaya, Marcus at the grid) washing the view white for a moment.

## Projectile (launch 0x64e20, update 0x66d30, impact 0x65f80, done 0x65e60)
- The projectile is a bone of the firing ship's model (index +0x390[i]); launch shows it (bone flags +0x102) and gives
  it its own matrix. Start: at the ship minus forward x +0x100, speed = craft forward speed x +0x198 % + +0x19c,
  inherited velocity (0x64fae..0x6503e, per projectile at its entry +0x30 + 12 x i) = (the craft's velocity, state
  block +0x3c, - the forward speed +0x990 x the forward axis, craft +0x1d0 + 0x20) x +0x180 x 0.01: the craft's
  sideways and vertical motion, its forward part being in the speed.
- Every frame: age = now - launch; age > +0x140 -> expired. Homing: other crafts within +0x188, 1 in 8 chance a frame
  to lock. Speed += dt x +0x174 up to +0x178; drop += dt x +0x17c. After +0x194 with a target: forward turns towards it
  by clamp(dt x +0x160 x 60, 100) %. pos += forward x speed x dt; pos.y -= drop x dt.
- Launch (0x64e20): the start point is taken from the bone first; then a lock-on record with +0x167 and a target is
  turned to look at it (0x65a7d: 0xb4e70 from the bone to the target, up (0, 1, 0)); a pitched one (+0x13f) instead
  takes the craft's own axes (+0x1d0) turned by mat_rotation_x(-clamp(+0x18c, 0, 90) degrees), nose up, keeping its
  position (0x65b0e): the bone's own turn on the ship is dropped.
- Speed (0x66f33): += dt x +0x174 up to +0x178, but a bouncing record with +0x156 that has bounced keeps its speed. A
  target knocked out (+0x1145) is dropped (0x66fd9).
- Craft hit (0x683b0): closest approach of each other craft to the step segment within the hit radius -> hit: 250
  starburst sparks ("Explosions"), damage 0x6acf0 (unless +0x7c & 0x400). Any craft within 300 m of the projectile
  (distance squared < 90000, 0x1a3a08) is flagged in danger (+0xb5).
- World: ray old -> new (0xd4770). A hit with no craft hit this step: a side-hugging record (+0x154) is put +0x15c off
  the wall (the hit point - the hit normal x +0x15c, 0x674e7) and flies on; else a bouncing record with bounces left
  bounces (kind 4: the bursts, 0.4 m (0x1a3550) off the surface, the frame along the reflection 0xb4f20 -> 0xb4e70,
  the drop cleared), else impact kind 2.
- Hug rays at +0x15c (0x67753): below along -up (+0x152) and above along +up unless already hugged (+0x155), each
  putting it that far off what it meets and clearing the drop; the sides along -right, then +right (+0x154).
- Frame (0x67add): when the speed is above +0x148 or the record bounces, and it did not bounce this step, the frame
  is rebuilt (0xb4e70, up (0, 1, 0)) along the move, or, having hugged, along the move with its part along the last
  surface's normal taken out. So a projectile noses down as it drops and follows the ground it hugs.
- Inherited velocity (0x67c7c, at the end of the step, after the frame turn; records without bounces, +0x165 clear):
  pos += it x dt, then it x= 1 - dt x 0.4 (0x1a3550). +0x180 is 0 in all 176 records of the stock weapon.nwp, so it
  only shows with modified data. Argon: argon::weapons::inheritedVelocity / decayedInherited (logic\shot_motion),
  weapons::carryInheritedVelocity (argon used to add the whole craft velocity x the percent every step, without the
  decay, bouncing records too).
- Impact 0x65f80 (kind 1 craft, 2 world, 3 expired, 4 bounce) and its explosions ("Explosions"), then for kinds 1..3
  the special's impact sound (slot 9, 0x67e7c) and 50 starburstred sparks, then done.
- Argon: Weapons::stepShot / steerShot / bounceOff (weapon_shots.cpp) over logic\shot_flight.hpp (worldHitAlong,
  hugSurfaces, bounceOff, aimAtLaunch, homing; tests\shot_flight_tests.cpp), logic\shot_motion.hpp (the frames, the
  turn along the path, the speed; tests\shot_motion_tests.cpp).
  Before, argon kept the bone's launch frame (no pitch, no aim), turned only on homing and bounces (so a ground
  hugger met rising ground and blew up), had no side hugging (Jaya's SPECIAL blew up on walls), sped up after
  bounces and threw the craft hit's 250 sparks at 30..90 m/s. Argon turns the side hug's wall normal back
  towards the shot (which way the game's collision normal +0x81d8 points is not settled).
- Trail (0x66650): a particle every +0xfc along the path, velocity back along forward x +0x104 + ((rand & 255) - 128)
  / 128 x +0x108 per axis, life +0x128, colour +0xf4 (type 1 / 2 / 3 = the starburst / starburstred / bubble
  systems); types 4.. are sprites (below).

## Trail sprites (0x66650 -> 0x9d000, update 0x9c540, drawn by the sprite pass 0xbd8f0)
- Type t > 3: a sprite in the spray pool (0x240540, 4096 entries of 0x50, shared with the water spray) via 0xbd670
  (EAX 2; the puffs use 1): the track's envdata\quadsprite.dds, cell t - 4 of a 4 x 4 grid of 128 px (u = 128 x
  (cell & 3), v = 128 x (cell >> 2); uv x 1 / 512, 1 / 1024, 0.97 of the half cell around its centre), list A (alpha
  blend) with +0xf0 flag 2, else B (additive 1, 1). 0x9d000 returns false when the pool is full, ending the trail's
  loop for the frame. EAX 2 faces the camera; the turn (0x9c540's angle slots) spins a positive spin clockwise on
  screen, the cell shown upside down and mirrored at angle 0 ([effects/sprites.md](../effects/sprites.md) "Sprite pool record").
- Spawn: pos / vel as the point particles; life +0x128; colours +0xf4 -> +0xf8; fade +0x124 (+0x224 in the pool);
  growth +0x118 (+0x228 / +0x22c); spin = +0x11c + (rand & 0x3ff) / 1023 x +0x120, negated on rand & 1 with flag 1;
  half size = +0x110 + (rand & 0x3ff) / 1023 x +0x114 (+0x210 / +0x214); flag 4 -> +0x241 fade in; mode +0x242 = 0.
- Update (each frame, `now`, dt): dead at age >= life; pos += vel x dt. Colour: after the fade time k =
  clamp(round((age - fade) / (life - fade) x 255)), (from x (255 - k) + to x k) >> 8 per ARGB channel; before it,
  with flag 4 and a fade > 0, k = age / fade x 255 scales the RGB (additive) or the alpha (blended), else `from`.
  Size: W, H x= 1 + (grow - 1) x dt^2 x 20 (0x1a35d8). Quad: corners (-+W, -+H) turned by age x spin degrees in the
  screen plane, plus the start angle +0x234 (0x9d000 arg 9: 0 from 0x66650; the hover dust, [effects/README.md](../effects/README.md) "Dust", passes
  rand % 360).
- Data: nearly every projectile uses them (types 4..19; ROCKET 7 = cell 3, NUKE 4 = cell 0, RAILBLAST 19 = cell 15).
- The impact's flash (+0xd4 flag 4, 0x66606) and the explosions' (0x7df66) are 0x9d000 sprites too, in cell 15
  ("Explosions").
