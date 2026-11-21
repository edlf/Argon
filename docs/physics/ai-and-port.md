# Physics: the AI pilot and argon's port

## AI pilot (drone_update)
- Look-ahead: 16 nodes from the current one (0xe47f0), branches chosen by surface priority (0xe3460).
- Tactics (0xe4be0): points at radius 1, 1+e+x, 21+e+x from the craft along the look-ahead (0xe51a0);
  e = 0/0/20/40/40 by speed class, 0 on hairpins, halved on corner_slow; x = brain +0x47c, reset every frame
  by drone_update to the per-AI table 0x1b9648 (= 90 for all) and halved on hairpins (+ overtaking offsets).
- Control (0xe0810): f = |v|/250 (300 at speed 4, 350 at speed 5); aim = normalize(fwd + 0.6 v f^2/100)
  (straight: 0.6 v f 0.005); c = up x aim; error = c . dir(craft -> lerp(P1,P2,f)) - clamp(0.018 L.up, +-0.1) where L = angular momentum
  (velinertia +0x14 is state +0x54); +0x924 keeps the error before that damping (used by the steering assist);
  dead zone 0.02 (0.03 straights); steer = (|e|-0.02)*500 (slow/hairpin or f <= 0.5) else *200, clamped 0..100;
  airbrake request at |e| > 0.4 or hairpin (only cuts thrust for AI); throttle 100 when |e| < 0.45 or f < 0.25;
  airborne nose-down (forward.y < -0.6) -> climb.
- Engine: power = engine_power[speed] * catch-up % (aivalues.txt, 82..108 %), gain = engine_gain[speed].
- Revs (0x6ed90): revs = forward speed / gear ratio (speed-class gear set); with the shipped tables
  (ratios 0.7.., engine_maxrevs 900) the AI never leaves first gear. +0x9ac = engine sound value.
- Speed cap (0x6f900): forward speed <= absolute_max_speed[speed] km/h.
- Hull/boundary-plane push (0x6f280) is dead code (its test is d < 5 && d > 5).

## Port
`src\craft\craftsim.{hpp,cpp}` (GameCraft). The water depth with its waves (0xce6b0) is ported ([water/README.md](../water/README.md) "Physics",
src\logic\water_waves.cpp). The hull contacts 0xd3c70 are ported in src\logic\hull_contacts.cpp (tests\hull_contacts_tests.cpp):
argon's collision grid stands in for the span lists (SimWorld::spanTriangles, sorted by triangle index, filtered by
the widened 0.25 m spans; ties at a step, which in the game depend on the span cursor's history, count as meeting);
argon's odd strip triangles are (t+1, t, t+2), a rotation of the game's (t+2, t+1, t), so normals agree.
Argon's lap counter and forward-only node tracking are its own.
"Drive yourself" (argon only): a keyboard / gamepad pad fills the same control slots as the AI (+0x8f4: [0] throttle,
[9] brake, [6] / [5] steer screen right / left, [8] / [7] screen right / left airbrake, [2] dive, [3] climb; the
craft's +right axis is screen left because the data is mirrored), with [12] = 0 (no AI steering assist), full engine
power and no "stuck" respawn (Back / Backspace asks for the reset as the game's BACK).

Player pad (0x70ac0, `this` = craft, pad state from 0xb4800: XINPUT analog buttons A..RT as floats at pad +0x64..+0x80,
sticks at +0x180 / +0x184, digital bits at +0xda):
- throttle [0] = RT (+0x80) x 0.390625, clamped 0..100. Steering [5] (+0x908, left) / [6] (+0x90c, right) = d-pad 100, or
  stick X past +-12288: (|x| - 12288) x 0.0048828125. Dive [2] / climb [3] (+0x8fc / +0x900) the same from d-pad up / down
  and stick Y.
- airbrakes [7] (+0x910) / [8] (+0x914) = LT (+0x7c) x 0.390625, only on the side you steer towards (0 when not steering).
- the pad never writes the brake slot [9] (+0x918); only the race end (100) and replays do.
- Airbrakes (player only): 0x71bc0 smooths +0x978 / +0x97c towards [7] / [8] (up dt x airbrake_force_gain x 60, down
  dt x airbrake_force_falloff x 60); 0x60230 then, with s = clamp(forward speed, 0, 50): F +-= right x (front + rear) x a x
  s x 0.01, T +-= up x (front - rear) x a x s x 0.01 (+ for +0x978). Thrust loses ([7] + [8]) x 0.5 %.
- Player side friction 0x5ffd0 (vtable 0x54), ported: k = friction_<track|rough|loose|water|slippery>_side_factor
  (record +0x344 + surface x 4). Airbrake level +0xa18 jumps up to max(+0x978, +0x97c) and decays 150 / s;
  k' = (101 - level) x (1 / 101) x k. If forward speed >= 100 or k >= 2: F -= right x (v . right) x mass x k';
  else g = (100 - speed) x 0.01 and the factor is g x k' + 2 (1 - g). (A flag at 0x52c8bd forces 0.1.)
  The AI uses 0x6f720 with the friction_AI_* factors and no airbrake term.

Brake (player only):
- 0x71fe0 (player controller): brake value +0x980 += dt x brake_gain x 60 while slot [9] (+0x918) is held, capped at
  100; released -> 0. The AI zeroes +0x980.
- 0x60450 craft_brakes, only with +0x92c (3+ hover contacts): F -= normalize(v) x +0x980 x brake_amount, each
  component no larger than v x 25; if forward speed > 0.1, T += 2 x right x +0x980 x engine_noseup.
- No reverse: held at 100 below 5 m/s, 0x71fe0 runs a timer +0x984, and 0x60450 has a reverse push
  (if byte +0x988 == 1: F -= (forward speed + 20) x forward x mass x +0x98c), but nothing in the XBE reads the timer
  or sets +0x988 for a craft (only cleared at 0x588f0 / 0x720b3). The player's side friction is 0x5ffd0 (below).
