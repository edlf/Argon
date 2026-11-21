# Quantum Redshift craft physics + AI (from default.xbe)

Notes for porting the game's own ship physics and AI into argon.
Addresses are XBE virtual addresses (= Ghidra addresses in neon_stock).
Tools: `tools\cmm\ghidra\` (decomp_refs.py `t:<sec>` timeout, func_info.py, find_scalar.py,
stats_offsets.py, stats_users.py, xbe_consts.py = read FLOAT_/DAT_ constants from the XBE).

| file | what |
|---|---|
| [ai-and-port.md](ai-and-port.md) | the AI pilot and argon's port |
| [hud.md](hud.md) | the in-race HUD |
| [opponents.md](opponents.md) | opponents |
| [rumble.md](rumble.md) | rumble |
| [standings.md](standings.md) | standings |
| [turbo-flaps-cameras.md](turbo-flaps-cameras.md) | turbo, flaps, engine sound, cameras |

## Classes (source paths in the XBE)
- `CGameObject` (object.cpp), `CPlayer`/craft (player.cpp, craft.cpp), drones (drone.cpp).
- Craft vtable at **0x19f3e8** (slot k at 0x19f3e8 + 4k; Ghidra lacked functions for some slots).
- Two craft classes on it: the player (CPlayer, vtable 0x19f3e8) and the drone (vtable 0x1a1c10; attract demo
  0x40310 builds craft 0 as a player, the rest as drones). 19 of the first 30 slots are shared; the drone's own:
  0 0x6f260 (player 0x721e0), 4 0x6dd50 (0x707d0), 7 0x60f80 (0x720d0), 9 0x6d480 (0x707e0), 11 update 0x6dd90
  (0x71490), 13 0x6d650 catch-up (0x70860), 14 turbo button 0x6f8e0 (player_pad_input 0x70ac0), 20 steering 0x6fb20
  (0x5f9d0), 21 side friction 0x6f720 (0x5ffd0), 27 turbo 0x6f9f0 (0x5e270), 29 race info 0x5ce70 (0x70810).
- Argon (craftsim.cpp): one `GameCraft` with a controller object, `PlayerController` (the pad, the brake / airbrake
  ramps, side friction 0x5ffd0, full engine power) or `DroneController` (the AI, 0x6f720, aivalues % x catch-up, the
  stuck respawn), set by `flyByPad(pad)` / `flyByAi()`. The game fixes the class when it builds the craft; argon
  switches it while racing (Drive yourself, the countdown's empty pad), so it is not a subclass.
- **Physics step 0x5d670** `(this=craft, short state, float dt)` (not a Ghidra function originally):
  sets `+0x11c = state`, clears force `+0x8c` and torque `+0x98` of the state block, then (each call followed by
  0x5d5e0 = clamp force/torque of both blocks to +-5e6):
  | order | vtable off | addr | what |
  |---|---|---|---|
  | 0 | - | 0x5dee0(dt) | prep |
  | 1 | 0x3c | 0x5e370 | ? (skipped when +0x1145 set) |
  | 2 | 0x40 | 0x5e6d0(dt) | **hover / ground probe** |
  | 3 | 0x44 | 0x5e600(dt) | ? |
  | 4 | 0x60 | 0x60450 | brakes / engine nose-up |
  | 5 | 0x4c | 0x5fe50 | dive / climb |
  | 6 | 0x50 | 0x5f9d0 | steering (turn_max, stiffening, flyingroll, flyingyaw) |
  | 7 | 0x54 | 0x5ffd0(dt) | ? |
  | 8 | 0x58 | 0x60230 | airbrakes |
  | 9 | 0x5c | 0x60760 | aerodynamics |
  | 10 | 0x48 | 0x5f800 | ? |
  | 11 | 0x68 | 0x60e40(dt) | ? |
  | 12 | 0x6c | 0x5e270(dt) | ? |
  | 13 | 0x70 | 0x61340 | ? |
  | 14 | - | 0x5aa10, 0x5c700, 0x5be00, 0x5ba10, 0x5b610 | integrate / collide (to decode) |
- **Object update 0x5aeb0** (craft Update 0x71490 calls it first): copies controller inputs from `*(+0x854)`
  (+0x24,+0x14,+0x18,+0x1c,+0x20,+0x10) to craft +0xa00..+0xa14, `+0x113c = stats`, calls
  `controller(+0x850)->vtable[2](craft, dt)` (player pad or **AI**), 0x5ae20, 0x5cb30 (respawn / reset to restart
  point 0x597330 + slot*0x88), vtable 0x74 = 0x70810 -> 0x5ce70 (race info: wrong way, positions, HUD).
- AI nodes at runtime: array `*0x1b9bc8`, 0x54 B each, pos at +8; count `*0x1b9bcc`; next node 0xebc11(node, 0).
  Per-craft race slot 0x5972f0 + slot*0x88 (+0 current node, +0x40 restart pos).
- 0x70430 (vtable 0x14): end of step, copies the new state, clamps matrix/position.

## character_stats.txt -> stats record
Parser 0x72390. 16 pilots, record **0x3a4** bytes at **0x593358 + pilot*0x3a4** (craft `+0x1140` holds the pointer,
pilot index at craft `+0x83c`). Field offsets: `tools\cmm\ghidra\out_stats_offsets.txt` lists them relative to
Shield_Strength, which is record +0xc4 (so record offset = listed + 0xc4). Gears: 5 sets x 10 floats.
Columns in the file are pilots in this order: Tara, Yuri, Kumiko, Darius, Jordan, Marcus, Leah, Kukailimoku,
Alicia, Vladimir, Abi, Pharaoh, Kaitlyn, Jaya, Lukas, Kutoo. `global_game_speed` (0..4) picks the *1..*5 variants.

## Object state (double buffered)
Two state blocks of 0x78 bytes at object +0x2c, current = `short +0x11c` (next = cur ^ 1).
Block (relative to +0x2c + i*0x78): +0x00 position, +0x0c 3x3 rotation (rows), +0x30 angular velocity, +0x3c velocity
(+0x68 in object terms), +0x48 momentum (+0x74 block 0, +0xec block 1), +0x54 angular momentum, +0x60 force accum
(+0x8c), +0x6c torque accum (+0x98). Object +0x25c/+0x260/+0x264 = up axis (copied by 0x70430),
+0x250.. / +0x268.. = other axes.

## Hover (0x5e6d0)
For each of 4 hover points (from the craft shape, object +0x20 -> per-state point list):
- ray from point + up*(1.5 - ride_height) to point + up*(-1.5 - ride_height); start blended 50% toward the body
  centre, end 5% toward it.
- 0x0d4770 = segment vs track collision; hit = (collision +0x81cc pos, +0x81d8 normal, +0x81e4 mesh, +0x81e8 its
  triangle). Mesh -> surface type via collision +0x1428 table (12 B per mesh, indexed by +0x81e4: +0 type, +4 reset,
  +8 flag; the craft keeps +0 / mesh / triangle per ray at +0x860 / +0x870 / +0x880, 0x5f096).
- ground colour (0x5f0d7): each holding ray adds the vertex colour (0xa46a0) of its triangle's sampled vertex (a strip
  mesh: index triangle + 2, i.e. the triangle's last; a list: its first); the mean, alpha 0xff, is craft +0x8ac..+0x8af
  (0x5ebe6; kept when no ray held). The ship draw (0x59bc0) lights the craft by it: [models/surface-block.md](../models/surface-block.md) "Surface block".
- spring: depth = -|end - hit| clamped to [-2, 0]; force = stats(+0x118 dampening?) * obj(+0x14 mass?) * depth along
  the hit normal (x0.33 when game mode 0x10/0x11). Torque += (hitpoint - centre) x force * 0.125 (0x1950a4 = 5.0?).
- velocity along normal > 0 scales the force (<= 5: 0.5*0.2*v + (1 - 0.2*v), else stats +0x2b0 = rebound).
- up to 4 contacts; 3+ agreeing surface ids -> craft +0x858 surface (0 track,1 rough,2 loose,3 water?,4 slippery,5 loop?).
- dust from the rear rays (2, 3) on surfaces 1 / 2, 1 in 16 per substep: [effects/README.md](../effects/README.md) "Dust".

## Integration (engine, 0xcf630 -> 0xcffb0 -> 0xcedd0)
- Substep h = 1 / clamp(max(|v.fwd|/3, |v.right|/2, |v.up|*1.2), 0.1, 1000), at least 0.004, at most 0.02 s.
- Per substep: craft vtable[0] = physics step 0x5d670 (forces), `rigid_integrate` 0xcedd0, box points 0xd1620,
  matrix clamp, world contacts 0xd3c70, response 0xd05c0/0xd5140, swap the two state blocks.
- Box points 0xd1620 (shape = object +0x20, built by 0x6fdc0): corners R v + x (+0x38 per state) and the 24 box
  triangle normals R n (+0x50 body, +0x54 world). Its bounds (+0x6c..+0x80) come from a second corner set (+0x18)
  placed with **R^T** (physics_substep passes the transposed rotation), so the query box is not the box's own once the
  craft is both pitched/rolled and yawed.
- Box triangles (0x6fdc0 -> 0xd14b0 x 24, every face split along both diagonals, normal normalise(cross(c0 - c1,
  c1 - c2)), outward): (4,5,1) (1,0,4) (7,5,4) (4,6,7) (3,7,6) (6,2,3) (0,1,3) (3,2,0) (1,5,7) (7,3,1) (6,4,0) (0,2,6)
  (5,1,0) (0,4,5) (5,4,6) (6,7,5) (7,6,2) (2,3,7) (1,3,2) (2,0,1) (5,7,3) (3,1,5) (4,0,2) (2,6,4).
- World contacts `physics_world_contacts` 0xd3c70 (collision object, the result at collision +0x1c08 count / +0x1c0c
  records of 0x34 bytes; collision +0 = "any contact"):
  - span query 0xd3770 (CSpanHitCache, per-axis sorted span lists 0xdd4a0 / 0xdd5c0, AND of the axes, first per mesh
    then per triangle): spans are 0.25 m steps, a triangle's widened by one step (0xdd2d0: round(4 min) - 1,
    round(4 max) + 1); results in mesh, then triangle order. Triangle corners as 0xa47e0 reads them: a list
    (3t, 3t+1, 3t+2), a strip (t, t+1, t+2) even / (t+2, t+1, t) odd (the same winding as the renderer's).
  - 0xd0c00: the first **100** found triangles, plane n = normalise(cross(c0 - c1, c1 - c2)) (into the solid on the
    tracks), d = -n.c0.
  - 0xd0d80 for each box triangle: a world triangle with all three corners strictly in front of the box triangle's
    plane is dropped (the last one moved into its place).
  - 0xd0e50: for each world triangle the box triangles with a corner strictly in front of the world plane are kept,
    and once one is, every later one too; a world triangle keeping none is dropped.
  - 0xdb7f0 for each kept pair (world first): the edges of each triangle (0: c0-c1, 1: c1-c2, 2: c2-c0) cut by the
    other's plane at t = -(v_q + d)/(v_p - v_q) in [0, 1], point t p + (1 - t) q; exactly 4 cuts or none. The longer of
    the two segments gives the line; each segment is sorted along it. Disjoint: none. The box segment strictly inside
    the world one: **normal source 2** (the world normal), ends = the box segment. The world segment inside the box
    one: **source 0** (the box triangle's normal). Otherwise **source 1**, normal = normalise(cross(world edge, box
    edge)) (0xdc2e0); the ends are the world low and box high cut (edges of those two), or the box low and world high
    cut, where the shipped code stores the box cut's edge as the world edge and the world cut's as the box edge.
  - record (0xd4108..): +0 type 2 on a reset mesh (collision +0x1428 +4 == 1, 0xd4256) else 0, +0xc object,
    +0x10 source, +0x14 point = the middle of the two ends, +0x20 normal, +0x2c the squared distance of the ends
    (unused by the response), +0x30 surface type; sources 0 and 2 also count at collision +0x1400 + object x 4. At
    most **500** (0xd439e). No push-out anywhere: the box can sink in.
- physics_substep then binds the next state (0xd05c0, copies v to the handler's +0x18) and walks the records once,
  in order: type 2 sets craft +5; type 0 goes to 0xd5140 with the second argument 0, so only **source 2** contacts
  move the craft (sources 0 / 1 are generated and ignored).
- 0xcedd0: Iw = R Ib^-1 R^T, w = Iw L, R' = R + h [w]x R with the **z terms of [w]x doubled** (0xceb30),
  normalise columns (0xceba0) + cycling cross-product re-orthogonalise (0xeb9f0); x' = x + h v (old v),
  P' = P + h F, v' = P'/m, L' = L + h T, stored w' = Ib^-1 L' (body inverse inertia, not rotated).
- Body (0x70340/0x6fdc0): box 4 x 2 x 6 (corners 0-3 bottom, 2/3 rear, +z forward), mass = mass_density * 48,
  Ixx = (dz^2+dy^2) m/12, Iyy = 4.333 m, Izz = (dx^2+dy^2) m/12.
- Contact response 0xd5140 (n into the surface): x -= 0.005 (n.v^) n; if m(v.n) > 0: P -= (1.2,1.03,1.2) m(v.n) n,
  friction P -= 0.1 P^ ((-P^.n) 0.8 + 0.2) min(|P|,70) unless n.up < -0.5, v = P/m, x -= 0.005 m(v.n) n;
  L -= (r x n) m(v^.n) 0.013 0.33 with v^ the bound copy, r from the position as this contact finds it, the change
  normalised when longer than 1 (0xd55d2 -> 0x58360); r.up > 0 sets craft +0x18c |= 2 (not ported).
- Respawn 0x5cb30: ray from restart pos +2 to -20, placed **7.5 above the hit**.
- Start 0x5a270 (the race setup, per craft on its player_start locator): segment from the locator 30 down (0xd4770);
  a hit places the craft **2 above the hit point**, none keeps the locator's position; the locator's rows give the
  orientation. Argon: GameCraft::settleOnStart (before 2026-11-16 the start used the respawn's 7.5, so the grid fell).
- **Reset request (craft +5)**: 0x5cb30 checks it each frame unless the craft is out (+0x1145); set means +8 = 0.5 s
  (the RESET HUD), +0x938 = 0, +5 = 0 and the craft goes to the restart point (0x597330 + slot x 0x88). Set by:
  - a hover ray hitting a reset triangle (craft_hover 0x5eb81, the hit record's +0x81f0 from 0xd4c77), before any
    other test on the hit (steep walls too);
  - a hull contact with one (0xd3c70 -> 0xd4256 makes it contact type 2, which physics_substep 0xd0273 turns into
    +5 on that body, with no contact response);
  - reset triangles: the collision table collision_build_from_cmm 0xd2a80 builds (+0x1428, 12 bytes a triangle: the
    material's +0x14 surface type, its **+0x18 == 1 reset**, 1 if the material is not drawn) per collidable mesh.
    Every track has some (4..13 meshes): Bracken's hidden volumes, Mars' outer desert (drawn), ...;
  - upside down: +0x934 counts dt while up.y (state R[4]) < 0 and the craft is not on the ground (+0x92c), else 0
    (0x5b265); over 5 s with up.y (+0x260) < -0.8 out of water (+0x92e) sets +5 (0x5ddcf, also +0x17e0 = 0);
  - fallen: position y < -400 (0x1a3938, 0x5ddf8);
  - the player's BACK (pad bit 0x200 pressed, +0x89 & 2: player_pad_input 0x71023);
  - the drone's progress check (drone_update 0x6df3f, unless 0x52af9c / the race over 0x52af9f / the countdown
    0x52af9e): +0x1714 += dt; over 2 s it takes 2 off and shifts the history (+0x1718 <- +0x1724 <- +0x1730 <- the
    position +0x27c); under 5 m between the oldest and the newest (4 s apart) sets +5 and +0x1714 = -5 (the next
    check 7 s later), unless 0x52abe1 (never written: a debug switch);
  - the "cheat reset" debug message (0x5b1ab, string 0x1a0648).
- Argon (craftsim.cpp GameCraft::requestResets, the per-triangle flag argon::TriangleSurface): all of the above but
  the debug ones; BACK is the pad's Back or Backspace. The restart point is argon's own (the last restart node,
  dropped as 0x5cb30). tests\craft_reset_tests.cpp checks each trigger on a synthetic world.

## Forces (in 0x5d670 order; AI overrides marked)
| fn | what |
|---|---|
| 0x5e370 thrust | only with a gear and ground/water contact: F = power(+0x9e4)/ratio(set 1, gear); minus airbrake share 0.005*(L+R); limited by engine_gain when revs rise faster than the 10-frame average; coasting: -falloff/ratio; F along forward, torque -F*engine_noseup about right |
| 0x5e6d0 hover | 4 rays from the bottom corners, start = p+up(1.5-ride) pulled 50 % to the centre, end = p+up(-1.5-ride) pulled 5 %; depth = -|end-hit| in [-2,0]; F += n*normalgravity*m*depth (n = downward surface normal) times rebound damping when extending; T += (n x up)*5 + cross(r,n)*depth*5; walls (|n.y| < 0.2) ignored; 3 contacts = on ground, 3 equal surfaces = surface type (cmm surface block +0x14: 0 track 1 rough 2 loose 3 water 4 slippery) |
| 0x5e600 gravity | (k*normalgravity + (4-k)*flightgravity)/4 * m for k contacts (30 vs 190 for Abi) |
| 0x5fe50 dive/climb | airborne only, about right |
| 0x5f9d0 / AI 0x6fb20 steer | ground: up * s^2/100 * turn_max; air: flyingyaw about up + flyingroll about forward; AI adds up*error*(40/100/200, or 200/500/1000 when correcting) |
| 0x5ffd0 / AI 0x6f720 side friction | F -= right (v.right) m k, k blended towards 2 below 100 m/s |
| 0x60760 aero | downforce; airborne: dartiness/flightcontrol/autolevelling + air drag; ground: quadratic drag cod[surface]/100 per world axis, codramp when reversing |
| 0x5f800 angular damping | T -= sum((a x w) x a) * dampening_ground/air |
| 0x60e40 nosedrop | airborne, after nosedrop_delay |
| 0x61340 water | 8 corners, buoyancy, noseup; > 3 under = water surface |

AI craft read **record 0 (Tara)** for everything accessed through +0x1140: turn_max/gain/falloff/stiffening,
flight yaw/roll, dive/climb, aero, air damping and the gear/rev tables.
