# Physics: turbo, flaps, engine sound, cameras

## Airbrake flaps (visual; argon: src\craft\craft_flaps.cpp)
- 0x58e60 builds one ship class per pilot id (jump table 0x59754); each vtable (0x1a0254.., 5 slots) has its setup in
  slot 0 (bone lookups by name, 0xb9c40) and its part animation in slot 2 (the wake emitter 0x50960 first).
- A flap is turned by 0xb99f0 (bone, instance, model, angles): clears bone flag 0x40, local rows 0..2 =
  D3DXMatrixRotationYawPitchRoll(yaw = y, pitch = x, roll = z) (0xf3117) rows x the bone row scale +0x11c, the
  translation kept; the angles go to bone +0x110. Every flap bone's stored local rotation is identity.
- Shared 0x504a0 (Tara, Darius, Jordan, Marcus, Leah, Alicia, Zafir, Kaitlyn, Jaya, Lukas; pushed with 0.005, 4 and
  sign flag 0): per side a = opening (0..1); with the brake +0x980 > 0: a += 4 dt; else demand = (airbrake + steer)
  x 0.005 (left [7] + [5], right [8] + [6]), a -= (a - demand) x dt x 4, twice that while opening; angles (-a, 0, 0).
  Zafir (0x563b0) uses AIRBRAKE_LEFT and AIRBRAKE_LEFT1 as its two flaps.
- Vladimir 0x576c0: demand x 0.003, opening rate 8, closing 4; left yaw -a, right yaw +a.
- Yuri 0x57990 / Abi 0x51d40: demand = airbrake slot x 0.009 (no steer), rates 8 / 4, then a += e x 0.001 (Abi
  0.0003) with e (+0x1b0 / +0x1a0) = e x (1 - 3 dt) while a hover ray touches (+0x92d), else e += ((climb [3] - dive
  [2]) - e) x 3 dt; a in -0.9..0.9; pitch +a (Abi +0.4 a).
- Kumiko 0x54b90: one AIRBRAKE_MAIN, a += (max([7], [8]) x 0.01 - a) x 5 dt, 0..1, no brake term; pitch -a.
- Kukai (0x544f0) and Kutoo (0x54c80) move no flaps (their classes tilt PODLEFT_ / PODRIGHT_ instead, not ported).
  Not ported either: the other part animations of the classes (pods, nozzles, CANOPY lookups in 0x563b0 / 0x544f0),
  and the ghosts' flaps (replays set them from the event bits +0x838).

## Turbo
- Turbo scale +0x8e8 (race setup 0x58510): (Turbo level + 3) x laps; each frame the craft update sets +0xa3c (huddata
  +0xc) = the seconds and +0xa38 (huddata +0x8) = seconds / +0x8e8 x 3 (0x5cf3b). The HUD's fan reads +0xc (the
  seconds, 0x4a39d: as ported). Nothing found reads the craft's +0xa38: the +0xa38 that engine_sound_update reads
  (0xc52b7) is the sound manager's own curve minimum ([audio/ships.md](../audio/ships.md) "Ships"), not the craft's.
- Weapon effects on the thrust and the steering (freeze / engine off / jitter): [weapons/README.md](../weapons/README.md) "Effects on the craft".
- Seconds left: craft +0x8e4 (0 in the race setup, 0x3eb42), clamped 0..100, shown as "Turbo: %.1fs". The first
  lap-line pass after GO sets it to (1.5 + 0.5 x Turbo level) x laps in Time Attack (0x429c0 at 0x42e0b, called only
  by its update 0x43220; [frontend/README.md](../frontend/README.md) "Turbo"), so a Time Attack starts with a full turbo. The other modes add 1.5 +
  0.5 x level per counted lap (race_craft_lap 0x3d5a0, on manager +0x57c bit 1), the first crossing included: the
  manager counts it (0x6313d: counter == crossings - 1, both from 0, so a 2 lap race ends on the third crossing; the
  zero start is inferred from that, its write not found), so they start with one lap's turbo (App::stepLapLine).
- Laps: g_race_laps 0x52cdd8 (a byte), set by 0x3b0b0 as every game mode starts a race (callers 0x3fa4c, 0x40464,
  0x40fd1, 0x41b2c, 0x4265c): by the speed class 0x1bbbe8, Novice / Amateur 2, Expert 3, Master / Redshift 5; in game
  mode 4 (multiplayer) the Multiplayer Laps option (0x10: 3 / 5 / 7 / 9; Default: the speed class's). So with every
  upgrade at 7 the start turbo is 10 s at Novice / Amateur, 15 s at Expert and 25 s at Master / Redshift. Argon:
  logic\race_laps.hpp, App::setRaceLaps as the race loads (it was 3 laps at every speed before 2026-11-16).
- Gained (0x3d5a0, on each lap the craft completes): the player + 1.5 + 0.5 x its Turbo upgrade level (0x1bb0ec +
  character x 0xb0 + 1, 0..7) and the "Turbo Up" popup (hud0 message 0xc); an AI craft + a speed-class amount (0 / 0.5 /
  1.5 / 2.5 / 4, or 0.5 / 1 / 2.5 / 4 / 5 with its flags +0x9f8 / +0x9f9). Multiplayer adds the best upgrade at the
  start (0x41de0).
- Button (player_pad_input 0x70ac0): pad A = bit 0x10 of the pad bits (0xb4800 maps A 0x10, B 0x20, X 0x40, Y 0x80,
  White 4, Black 8, LT 1, RT 2); +0x91f is set only once the button has been let go since (+0x920). Black, on its
  press (+0x88 & 8, 0x71033), steps the player's tripod +0xe24 0 -> 1 -> 2 -> 3 -> 0 (close, mid, far, cockpit; jump
  table 0x71474). Argon: RB (xemu's Black) or C, logic\ship_views.hpp, App::followViewButton. +0x9f0 = held, the
  race running (0x52af9e clear), turbo left and airborne under 0.5 s (+0x938); starting it plays a ship sound (7).
- craft_turbo 0x5e270 (in the force list): with +0x9f0, turbo left, airborne under 0.1 s, no post-race / restart:
  +0x9f3 and +0x9f1 set, the seconds drain by the step (not with Infinite Turbo 0x1bbbf1), F += turbo_force<speed> x
  forward (character_stats turbo_force1..5).
- Speed cap (0x720e0 player / 0x6f900 AI): with +0x9f3 the cap is turbo_max_speed<speed> instead of
  absolute_max_speed, until the forward speed is back under absolute_max_speed (+0x9f3 cleared).
- AI use: 0x6f9f0 (the drone's piece 12) is the same with a cooldown +0x9c4 in front: +0x9c4 -= the step (0x6f9f6, at
  least 0) every call, and the turbo fires only while it is exactly 0 (0x6fa33); 0x5dee0 sets it to 1 (0x5df6c) with
  every blast push, on any craft, and the race setup clears it (0x587b6); nothing else reads it (the +0x9c4 reads at
  0xc3714 / 0xc3ed4 / 0xc8604 are another object's). So: no drone turbo for 1 s after a blast. Unlike 0x5e270 it does
  not check the race flags 0x52afa8 / 0x52abe8 (the button already does) nor Infinite Turbo (0x1bbbf1: the drone's
  seconds always drain), and pushes along its state block's rotation column rather than +0x250. The drone's button (+0x91f -> +0x9f0,
  0x6f8e0) comes from ai_turbo_decision 0xe10f0: not before the race progress (lap counter x 100 + % of the lap)
  passes HeadStartDistance[track][speed] + 100 (g_ai_values +0x120), only on a node with turbo_good_<speed>
  (+0x47b, CAIControl_flags 0xe0f40: masks 0x2000 << speed in the node's two flag words; turbo_never_<speed> 0x100
  << speed -> +0x47c, unused here); a trigger (speeds 1-2: +0x47d and the player 0..2.5 % of a lap ahead; speeds 3-5
  also +0x47d with the player further ahead and a slot flag, or +0x47e with the craft one place ahead within 2.5 %)
  arms a 0.5 s timer (+0x480) during which the button is pressed while the steering error (+0x474) is within
  +-0.05. Nothing in the game writes +0x47d / +0x47e (re-checked: a scan of every instruction's displacement finds
  only the reads at 0xe1166 / 0xe11b1 / 0xe1204; the flags function 0xe0f40 writes +0x478..+0x47c, and no dword write
  at +0x47c..+0x47e hits the CAIControl; a bulk copy or memset of the object is not excluded), so the triggers never
  fire and the AI does not turbo (unverified in xemu). Argon ports the cooldown (logic\blast_push.hpp
  TurboCooldown, GameCraft::droneTurbo via the controller's turbo piece) but not the decision: only `--turbo-test`
  presses a drone's button.
- Argon (craftsim.cpp turbo / turboInput / speedCap, app_results.cpp lap grant): Space or pad A; no upgrades (level
  0, so 1.5 s a lap); the AI does not use it (see above); the sound SF_SHIP_TURBOFIRE (ship sound id 7 in the
  name table 0xdda90; <pilot>.txt -> an_turbo_comp, NOPOSITION, MAXVOL -600 = -6 dB) restarts whenever the button
  is on and it is not playing (0xc4a70 = find a playing instance), so it repeats while held. Argon only: T (while driving) or the panel's
  "+5 s turbo" adds 5 s. Effects: the HUD turbo fan blinks white while pushing (red / blue 0xff on frames 0..3 of 8
  of the 60 Hz counter, else 0x40) and glows 0x40 + 191 x (1 - t) for a second after a grant (gamehud_render 0x4a2bc);
  the trails brighten ([effects/README.md](../effects/README.md) "Trail"); the speed blur ([effects/README.md](../effects/README.md)) kicks in past 720 km/h; the jets lengthen
  ([effects/README.md](../effects/README.md) "Jets"). `--turbo-test N` (with --drive) starts with N s and
  holds the button for the AI-driven test craft.

## Engine sound
`engine_sound_update` 0xc5200 uses only forward speed (smoothed 1 m/s per frame) as % of the class top speed,
not revs/gear/throttle; see [audio/README.md](../audio/README.md).

## Cameras
- Tripods per craft (dataports "craft%d tripod internal/externalclose/externalmid/externalfar"), fov 60.
- Internal (0x5aa10): eye = pos + forward*camera_forward + up*camera_up, look = eye + forward;
  up = upSmooth (object_update: halfway to the craft up each frame) - right * k * tilt, tilt follows the
  signed-squared (steer L - R)/100 * turn_headtilt_amount averaged over turn_headtilt_buffer frames
  (each entry *0.9 per frame) with turn_headtilt_gain / falloff * dt * 0.06; k = headtilt stiffening blend.
- External (0x5be00): eye = pos + up*above - forward*distance + right*sway*5, kept at `distance`;
  look = pos + up*(depthsum-2.2)*0.05, look.y += lookatheight*up.y; camera up -> world up (0.1/frame)
  unless the craft is upside down. Sway (0x5c700): 12-frame lateral velocity average * -0.01 * factor,
  clamp +-2.5, signed square, + 0.03/0.05 (up.L), asymmetric smoothing (x3 / x2 * dt).
- config\camera_<t>.txt ship cameras use ship space x forward, y left, z up; panrange = lap % window.
