# Audio: ships

## Ships — `audio\ships\<pilot>\`
`<pilot>.txt` maps events to folders (`SF_SHIP_TURBOFIRE an_turbo_comp`,
`SF_SHIP_ENGINEPOWERUP an_mstart`, weapon launch/travel sounds, ...).
`engine\<pilot>.ini` = layered engine loops:
```
Loop
AN_Eng_FX_5_FAST_LP.wav
Vol_0 0.000000 -1066.666748      <- speed (0..100), volume in mB (1/100 dB)
...
Pitch_0 0.000000 -8142.000000    <- speed, pitch in 1/4096 octave (±8142 ≈ ±2 octaves)
```
`h_engine\h_<pilot>.ini` has the same format: an extra set (fast loops + water loop) that the game
loads **only for the player's craft** (`craft_sound_bind` 0xc5700) and plays together with the normal
set; each set has its own mute flag in the sound object (+0x66e normal, +0x66f h_), both 0 by default.

How the game drives the curves (`engine_sound_update` 0xc5200, every frame, per craft and set):
- `S` follows |forward speed| (craft +0x990 via the `craft%d sound` dataport) by at most 1.0 (m/s) per call.
- `x = 100 * S / (absolute_max_speed[speed class] * 0.27778)` — % of the class top speed, clamped to the sound
  manager's +0xa38..+0xa3c = 0..500 (written once, in `craft_sound_bind` 0xc7593, with the volume range +0xa40 / +0xa44
  = -10000..0 and the pitch range +0xa48 / +0xa4c = ±8142). The curve walk puts (+0xa38, the first value) before the
  points and (+0xa3c, the last value) after them, so above the last point the last value holds.
- volume = linear interpolation of the 8 Vol points, clamped to -10000..0 mB, then scaled by the
  sound-effects volume option; pitch = linear interpolation of the 4 Pitch points, clamped to ±8142.
- The pitch goes to IDirectSoundBuffer_SetPitch (0xc550b -> 0x13abd7), which is absolute: 0 plays at 48 kHz
  (the MCPX's base rate), not at the file's rate. The layers are 22050 Hz, so the game
  plays them 48000 / 22050 (1.12 octaves) faster than their own rate at pitch 0; argon played them relative to the
  file until 2026-11-20 (an octave low). Checked on a real Xbox capture (XboxHD+, Silver, Tara, the grid): the
  predicted idle engine (both sets at x = 0, -7 dB path, Engine Volume -4 dB, the front HRTF pair of 2048) is within
  1.6 dB of the capture's 60..125 Hz band, and only the absolute pitch fits its 1..2 kHz band. That prediction left out the five AI engines, which xemu plays at full on the grid: with them it ran about 6 dB hot, the 3D mixbins' headroom ("Mix levels").
- Revs, gear and throttle are **not** used by the engine sound.
- The turbo does not change the engine sound. (An earlier reading took 0xc52b7's +0xa38 / +0xa3c for the craft's
  turbo fields at the same offsets; `esi` there is the sound manager, whose per-craft S values run +0xa1c..+0xa34.)
- Weapons (0x6b25b / 0x6d261): a hit that lengthens the engine-off time calls 0xc2ef0 (SF_SHIP_ENGINEPOWERDOWN,
  `<x>_mshut`), the time counting down to 0 calls 0xc2f40 (SF_SHIP_ENGINEPOWERUP, `<x>_mstart`); a TSHIELD that
  clears its owner's effects (0x6a4df) plays neither. The byte +0x201 + craft of the sound manager (0x4484d8, in BSS,
  written only by these two) lets each play only after the other, so a craft slot's first engine-off hit since boot
  is silent. Argon: logic\engine_power_sound.hpp + Weapons::afflict / restartEngineAfter, the player's craft only (no
  positional ship sounds); the folders `NOMAIN` with `LEFT` / `RIGHT` (some `LS` / `RS`) files, mixed to stereo.
- The ship sounds come from the pilot's `<pilot>.txt` (audio\ship_sound_list.hpp, readShipSounds): the folder names
  differ per pilot (darius `do_turbo_comp`, `do_mstart`, ...). Ids (parser 0xdda90): 0..3 WEAPONTRAVELPROJECTILE /
  LASER / SPECIAL / RADIAL, 4 PROJECTILEFIRE, 5 MEGAFIRE, 6 LASERFIRE, 7 TURBOFIRE, 8 TURBOFAIL, 9 ENGINEPOWERUP,
  10 ENGINEPOWERDOWN; craft k's are the sound table's entries 0x25 + (track sounds) + 11 k + id.
- Played: PROJECTILEFIRE at a projectile weapon's launch and LASERFIRE at a beam's (weapon_launch 0x64e20, by the
  record's class), at the shooter; ENGINEPOWERDOWN / UP at the hit craft (0xc2ef0 / 0xc2f40). Never played: MEGAFIRE.
- Travel loops: each launch also queues its loop with its launch sound (0x650aa projectile, 0x651ab beam, 0x65399
  shield: only with RADIALLAUNCH, so craft 0 / split screen) through 0xc9d10 (key: craft slot, weapon object +0x14,
  projectile; kind = class): one of 10 entries at sound manager +0x133c (0x18 each: key, kind, due +0x10, queued
  +0x14), due 0.5 s later on the race clock 0x50e430 + 0x50e434 (+0x9b8 = 0.5, set only by the defaults 0xc96e0).
  With all 10 queued, the one queued earliest starts at once and gives up its entry. sound_frame_update
  (0xc4f25 -> 0xc9cb0) starts the due ones: 0xc9dc0 takes one of 10 voice slots at +0x1274 (0x14 each: key, voice,
  kind) whose voice is 0, else stops the one started earliest (voice +0x1c); it plays sound 0x25 + (track sounds) +
  11 slot + WEAPONTRAVELPROJECTILE / LASER / RADIAL for class 0 / 1 / 2 (SPECIAL for any other kind; none is queued):
  the shooter's own pilot sound, at the shooter, MAXVOL, culled beyond MAXDIST like any positional sound (a culled one
  leaves its slot free). 0xc9f80 (key, position, radius) sets the voices' position (0xcacd0; the listener in split
  screen): every projectile update (0x66d30 at 0x671a7, after the move; radius 0) and the shield update (0x69970 at
  0x69cd2, while the sphere's ages +0x184 < age < +0x188 and the race is not over 0x52af9f) at the shield bone with
  radius = the record's +0x190, the sphere's **final** radius (the growing one is computed after the call). For kind 2
  it also sets MINDIST = radius + 200 (0x1a35a0) and MAXDIST = (sound MAXDIST - MINDIST) + radius + 500 (0x1a359c).
  The beam update (0x68620) never moves its loop: a beam's stays at its craft. 0x65e60 (a projectile, beam or shield
  done: the impact 0x6809f, the beam's end 0x68b07 / 0x68b3b, the shield's life +0x140 run out 0x699cd unless the race
  is over, a repeating volley's relaunch 0x69eba) calls 0xc9f00 (0x65f46): its voices stop and its queued entry goes,
  so a projectile done within 0.5 s is never heard travelling. 0xc9c30 stops all and empties the queue (race restart
  0x3e9c4, LOSEMENU 0x84c39, POSTABLE ON 0x8eab2, 0x421b2, 0x879fc). Folders: tara TL_PTravel (LOOPS 1, TYPE WEAPON:
  MINDIST 600 / MAXDIST 700), TL_RTravel (LOOPS 1: 20 / 200, so a TSHIELD's 100 m sphere gives 300 / 780); no pilot
  list names a WEAPONTRAVELLASER folder, so beams are silent.
  Argon: logic\travel_loops.hpp (TravelLoops: the queue and the voice slots by key, travelSound, radialTravelDistances;
  tests\sound_rules_tests.cpp), app_race_sounds.cpp (queueTravelLoop from playLaunchSound, startDueTravelLoops in
  stepRaceSounds, move / spreadRadial / stop / stopTravelLoops), weapon_shots.cpp stepShot (move), weapons.cpp (stop:
  a shot done, a relaunch), weapon_hits.cpp updateShieldSpheres (follow the shield bone, stop at its life),
  stopTravelLoops from resetWeapons, the replay start and openResultsTable; RaceSfx::moveTo / setDistances. Keys:
  (shooter, weapon.nwp slot, projectile); the clock is the race clock (simClock). Not in replays (argon's replayed
  launches play no sounds). A craft raising another shield while one is up stops the old one's loop (argon keeps
  one shield a craft; the game would run it out).
- Turbo (player_pad_input 0x70d92, each human player): the button held with turbo to use starts TURBOFIRE unless it
  plays, and the voice rounds the clip's `smpl` loop (tara: 11776..23411 of 37569 frames) while held; held without
  turbo, TURBOFIRE stops and TURBOFAIL plays unless playing: the same clip from its loop's end (0xc3540, the wind-down
  tail; no pilot list names a TURBOFAIL folder); let go, both stop. Argon: RaceSfx::setTurbo, RaceSounds::stepTurbo.
- 3D: craft 0's engine is placed on the listener; every other craft's (0xc3220: its normal set) at its position
  with MINDIST 50, MAXDIST 200, the rolloff curve and a doppler factor of 3. In game mode 4 (split screen) every
  craft's engine sits on the listener (0xc5510), so each player's is heard centred. Argon: Audio::playOtherEngines
  for grid slots 1..n, App::placeOtherEngines (curve input, position, velocity), audio\hrtf.cpp (direction), audio\spatial.cpp (doppler).
Ported in argon (`src\audio\audio.cpp` + `app_drive.cpp stepDrive`).
