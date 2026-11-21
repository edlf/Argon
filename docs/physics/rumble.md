# Physics: rumble

## Rumble
- 0x5b610 (per craft, each frame): ground contact g (+0x9c8) += 10 dt on the ground (+0x92c), -= 2 dt in the air,
  0..1. a = (2 (s - f) + s) x 0.005 with s = |v| (+0x9a4), f = forward speed (+0x990); amp = a^2 x rumble_<surface>
  x 0.2 x g, at most rumblelimit_<surface> (character_stats, per pilot at stats +0x2c8 / +0x2dc + surface x 4;
  surfaces track, rough, loose, water, slippery). rumblehertz_<surface> (+0x2b4) paces a random camera shake
  (+0x9cc..+0x9d4, timer +0x9d8, below). Motor +0x9ea = amp x 300000 + +0x9dc (decays x (1 - 4 dt)); over 65000: 65000
  and +0x9e8 gets 20000; +0x9ea is 0 on frames where rand() > 0x3fff. +0x9e8 += clamp(+0x8b0 x 1000, 0, 65000)
  (+0x8b0 decays x (1 - 2 dt) while > 0, and also adds a random vector x +0x8b0 x 0.01 to the camera shake); past
  +0x890 > 2.2, +0x9e8 -= (+0x890 - 1.5) x s x -25 (16-bit arithmetic). Both are fistp'd to a qword and their low
  word kept: the words wrap (20000 + a 65000 knock gives 19464), they are not clamped. Both 0 while 0x597362 + craft
  id x 0x88 is set.
- +0x9dc: a beam's first frame on its target adds 50000 (beam update 0x68620 at 0x68f5a / 0x691c1, 0x1a3a04).
- +0x8b0, the knock: an explosion (0x7de80, every explosion carrying a weapon's effects, i.e. not burst flag 0x10)
  calls 0x7e410, which adds a push to +0xa24 of every craft but the shooter's: along normalize(craft - explosion) (x
  + 1 when they coincide), size = the weapon's area falloff (inner % within the inner radius, from 0.01 past it linear
  to the outer % at the outer radius, 0 from there; area weapon or not: a ROCKET's 5..15 m at 100 % pushes too) x the
  explosion type's push scale (weapon.nwp's explosion table after the pilots, 0x1e8bb8 + type x 0xa4 + 0x18: 100 for
  the types used, 0 for type 14) x 0.01. The next physics step's first piece 0x5dee0: unless the craft's shield weapon
  is up (then it is only cleared), the push's y is made positive, +0x9c4 (the drone's turbo cooldown, "Turbo") = 1, **+0x8b0 =
  |push|**; the push's part across the velocity (the current block's +0x3c, + 1 on x when 0) x 0.02 (0x1a36d8), 0..0.4
  (0x1a3550), doubled (0x5e0ba) for crafts with id +0x1c > 0 in game modes 0 / 1 (0x52ccfc), takes that fraction x
  the velocity off the momentum of both state blocks (+0x74, +0xec: block +0x48; mass 0.02 x 48 = 0.96, so up to 42 %
  of the speed, 83 % doubled, the next integration v = P / m). Not the push itself: the craft is never moved along
  it. Then |push| x the right axis +0x268 x -10 (0x1a372c) x +0x8f0 goes into the step's force (block +0x60):
  subtracted on an even rand() (0x5e16d, `test al, 1`), i.e. a shove to the right, added (to the left) on an odd one;
  +0x8f0 = 1 for the player (vtable slot 9 0x707e0, at 0x707ed), 2 for the drone (0x6d480, at 0x6d4a3). It runs at the
  start of 0x5d670 after the force is cleared, so the shove lasts one substep (4..20 ms; |push| 100 x 10 x 2 x 4 ms
  = 8 momentum, about 30 km/h sideways). +0xa24 is cleared.
- The camera shake (0x5b610 each frame): three rand() calls (0x5b6ae / 0x5b6d0 / 0x5b6f2) give a direction d, each axis (rand() -
  0x3fff) / 32767 (0x1a3748) x 0.1 (0x1a3508), so +-0.05. The timer +0x9d8 -= dt, the shake x (1 - 2 dt); when the
  timer is <= 0 (0x5b7f4) it gains 1 / rumblehertz_<surface> (at least 0; a hertz of 0, x87 division: infinity,
  never runs out again) and the shake = amp x d (amp: the motor's surface amplitude). While the knock is above 0 it
  decays x (1 - 2 dt) and the shake += d x knock x 0.01 (0x1a355c): a 100 knock shakes by up to 0.05 a frame, fading
  over about 2 s.
- Only the internal camera reads it: its update 0x5ba10 (skipped while a replay plays, 0x52afac) ends with the
  tripod's up (+0xca0) += (shake x, 0, shake z) and its look-at point (+0xcac) y += shake y (0x5bdc5..0x5bdf5). The
  chase cameras 0x5be00 / 0x5c700 never read +0x9cc.
- The contact jolt +0x8b4 (0x5ba10 too, before the shake; the calls at 0x5daf8 / 0x5dafe run 0x5ba10 then 0x5b610 for
  every craft): x 0.95 (0x1a3620) a frame (per frame, not per second: half gone in 14 frames at 60 Hz), then += 0.5
  (0x1a3504) x the momentum change of each of the frame's contact records whose normal . direction is above 0.25
  (0x1a3518). The records (0x442e94 + craft x 0x454, 25 entries of 0x2c bytes; cleared by 0xd09e0 at the start of
  physics_world_step 0xcf630): each response in physics_substep (0xcffb0, world contacts only, not craft pairs) adds
  the new state block's momentum change (+0x74 after - before; +0x0), its contact point (+0xc), normal (+0x18) and
  normal . normalize(the current block's velocity +0x68) (+0x24) to the current entry and counts it (+0x450); at the
  substep's end 0xd0a30 (0xcff92) averages +0xc / +0x18 / +0x24 by that count and advances the entry index (+0x44c)
  up to 24, which is the count 0x5ba10 reads, so one record per substep that had contacts and at most 24 a frame.
  Then the tripod's up (+0xca0) += right (+0x268) x clamp((jolt . right) x -0.003 (0x1a3734), -0.1, 0.1) (0x1a366c /
  0x1a3508) and its look-at point (+0xcac) += up (+0x25c) x clamp((jolt . forward +0x250) x 0.01 (0x1a355c), -0.1,
  0.1). With the look-at 1 m ahead (tripod(3)), a hard landing or a head-on hit (momentum change backwards) dips the view by
  up to 0.1 (about 6 degrees) and a side hit rolls it a little; the jolt's part along the craft's up is not used. Skipped with the rest of
  0x5ba10 while a replay plays (below).
- +0x890 = the hover's compression: craft_hover 0x5e6d0 clears it (0x5e734) and subtracts each ray's depth (0..-2,
  also kept per ray at +0x894..+0x8a0, 0x5ee31): the four depths summed, 0..8 (argon's depthSum_).
- craft_update 0x71490: +0x1138 x (1 - 10 dt), +0x1134 x (1 - 5 dt) (dt at most 0.1); each contact of the craft in
  0x442e98 (0x2c-byte entries) adds |its vector| x 1000 to +0x1138 and that x its +0x20 factor to +0x1134; each craft
  pair touching it (0x4479dc, 3 ints: value, craft a, craft b) adds value x 1000 to both; clamped 0..65535.
- player_pad_input 0x70ac0: motors (first, second) = (+0x9ea + +0x1138, +0x9e8 + +0x1134) (fistp, low word) via
  0xb46d0, which keeps the first only above 0x4000 and the second only above 0x2000 (else 0; the 0x34bc / 0x2134
  hysteresis before it is overwritten by the last test). 0xb4530 sends pad +0x194 / +0x196 when they changed, as
  XINPUT_FEEDBACK (+0x1e) Rumble.wLeftMotorSpeed (+0x60) / wRightMotorSpeed (+0x62): **first = the left,
  low-frequency motor, second = the right, high-frequency one**. A knocked-out craft (+0x1145, set with the time
  +0x1148 by 0x58d20) sends round((3 - t) x 33) x 300 to both for 3 s, then 0 ([frontend/options.md](../frontend/options.md) "Settings effects").
- Argon (craftsim.cpp GameCraft::rumble, logic\rumble_rules.cpp, tests\rumble_rules_tests.cpp): as above with the
  words wrapping; +0x9dc from the beams (Weapons::stepBeam -> addBeamRumble), the knock from the blasts
  (Weapons::pushCraftsFromBlast, weapon_damage blastPush, the explosion table's push scale read by
  WeaponBook::readExplosionTypes; the shield is checked at the blast rather than at the next step), +0x890 = depthSum_. The
  contacts' impulses (the wall response's normal impulse, the craft-pair J) stand for the game's contact vectors,
  factor 1. 0x5dee0 is GameCraft::takeBlastPush at the start of GameCraft::forces (logic\blast_push.hpp: upward,
  brakingFraction, doubledBraking, sidewaysForce; +0x8f0 = the controller's blastSideScale, so an AI-driven player
  craft uses the drone's 2; the doubling is set at the blast from the slot and GameLogic::doublesDroneBlastBraking).
  The shake is logic\camera_shake.hpp (CameraShake, run by GameCraft::rumble; applied by tripod(3), the cockpit view);
  tests\blast_knock_tests.cpp. The drive log (`--drive-log`) prints the player's knock and shake, `--autofire` logs
  each "blast push" (slot, size, speed). The jolt is logic\camera_shake.hpp (SubstepContacts recorded by
  GameCraft::collide from each response's momentum change and its normal . direction, ContactJolt started by
  GameCraft::frame, `jolted` applied by tripod(3) before the shake); the drive log prints it along the craft's right /
  up / forward ("jolt r/u/f"). A drive on Solar (AI, speed 3) gives forward -28 / right +8 on a landing at 8.6 s (the
  look dip clamped at 0.1), Tokyo right +6.6 at 7.2 s; before the port the cockpit view had none.
- In a replay (0x52afac): 0x5ba10 returns at once (0x5ba16 / 0x5ba1d, to the epilogue 0x5bdfb), and the tripod is not
  left holding the last frame's offsets: 0x5aa10, run just before it by the same step (0x5dae3) with no replay check
  on that path (its 0x52afac tests at 0x5ad1b / 0x5addc only gate player 0's sound), writes the up (+0xca0..+0xca8,
  0x5ac5e..0x5ac98) and the look-at point (+0xcac..+0xcb4, 0x5acaf..0x5accf) afresh. So the cockpit view in a replay
  shows neither the jolt nor the shake (the race step 0x7f3a0 calls the world step 0xcf630 at 0x7f438 with no replay
  check, so the step runs in a replay too; not checked live). 0x5b610 has no replay check (0x5b610..0x5ba05): the shake (+0x9cc) and the
  ground contact (+0x9c8) still run, unread; the jolt +0x8b4 is neither decayed nor fed, so it keeps its value.
- Argon: GameCraft::placeFromReplay clears the jolt and the shake (clearCockpitJoltAndShake), so tripod(3) of a
  replayed craft (View Replay, the attract demo, the ghost replays, the drones) adds neither, and the drive log prints
  0 for them while a replay plays (checked: `--menu --quick-race 5 --race-go --ai-race --countdown-at 0 --view 3
  --drive-log 1 --drive-time 200 --view-replay --shot <png>`, the race's shake up to 0.007 then 0 through the replay;
  tests\craft_reset_tests.cpp aReplayedCraftHasNoCockpitShake). The game's still-running 0x5b610 state is not kept
  (nothing reads it during the replay; a race after it starts from the grid).
