# Replays: who uses the event buffer

## Users
- **Attract demo** (game mode 2, CAttractDemo vtable 0x1a1a0c; [frontend/intro-title.md](../frontend/intro-title.md) "Attract demo"): attract_init 0x40440
  opens `z:\replay<n>` as the CReplay "demo" (+0xd2d4), registers the crafts and "WeaponEvents"; attract_update 0x40af0:
  eventbuffer_advance to the clock, craft_replay_inputs for each craft while it returns 0; at the end (or on a
  button) GAMELOGIC message 7 back to the front end.
- **View Replay** (WINMENU / LOSEMENU): 0x3e8e0 (game logic vtable +0x2c) closes the menu ("WINMENU OFF" to WINMENU,
  0x3e930), resets the crafts to their start positions (+0x2ac), the AI, the pickups and the clock to 0, restarts
  the music (below), and "restart" to craft0; the race's own buffer then plays: 0x3e700 (from 0x40150) each frame advances it and replays the
  crafts; at its end (advance non-zero, after the first second) the post-race menu comes back (WINMENU if the player
  won, else LOSEMENU; 0x52afa8 = 1 / 2); with 0x52afa8 already set (a later viewing) the replay restarts
  from 0 instead (vtable +0xc, 0x3f730).
  - Music: with 0x4407fc clear (taken to be the custom soundtrack's flag: the other branch only restarts the stream) 0xc8e10 (the race's own restart: the stream stopped, the
    track folder, mode 0, state 2 = start) then music_set_mode(1) before the music's tick, so the track's on_start.wav
    plays first, then the segments (each twice) as in a race; with the custom soundtrack only the state 2 (its next
    song). Nothing at the replay's end (the music plays on under the menu); a later viewing calls 0x3e8e0 again. The
    replay's own events may still move the mode (the lap / place / shield cues are not held back by 0x52afac; not
    checked). The attract demo: attract_init 0x405e4 calls 0xc8e10 alone (mode 0, intro.wav), no mode 1.
  - HUD: WINMENU OFF (0xa0a4f) / LOSEMENU OFF (0x84edb) send hud0 "set all", so the HUD shows while the replay
    plays; their ON (WINMENU / LOSEMENU sent back by 0x3e700 at the end) sends "set none" (0xa09a4 / 0x84e30). The
    attract demo (0x4057d) and the ghost replays (0x436f1) send "set none". The power-up panel reads the replayed
    player craft's own levels (huddata +0x74.. from 0x6bd70): no entry carries them, but craft_weapon_update's pickup
    test (0x6d0e6 -> 0x6ba30) is not held back during a replay (no 0x52afac check), so the craft is credited as it
    passes each pickup again; the replayed fires (0x6a390) do not clear them (only weapon_select 0x6c33e.. and a
    knock-out 0x6a7d5 do), so the panel fills and stays full.
- **Pause menu ghost replays** (Time Attack): the items' handlers (0x89bf4 Replay Loaded Ghost, 0x89c07 Replay
  Session Ghost) call 0x8b560, which only closes the pause (0x782d0, as Continue; 0x448e50 is the pause menu's own
  state, 1 paused / 3 closed, set by Continue and Restart too), and set +0x626 / +0x627; the pause update (0x8a28a /
  0x8a347) then sends GAMELOGIC message 3 (loaded) / 4 (session) (message vtable 0x1a1a3c, +0x20 the value). Time
  Attack's message handler (vtable 0x1a16f8 [8] 0x424d0) sets +0xd2bc = 1 / 2; its restart (vtable [3] 0x43460) then
  sets the clock to 0, opens `z:\eventbuffer\preloaded` (1) / `fastest` (2) as the CReplay "replay" (+0xd3a8) with
  the replayed craft +0xd4c0 = 2 (the loaded ghost's craft +0xce74) / 1 (the session ghost's +0xce70), places the
  player's craft (+0xce6c) and the ghost craft at player_start0 (0x59f90), camera0 "replay on" / "replay0output", hud0
  "set none". Its update (vtable [4] 0x43770) hides all three crafts (+0x1129) but the replayed one, advances the
  buffer to the clock, replays the player's (hidden) craft from it (craft_replay_inputs) and copies its two state
  blocks into the shown ghost craft; it ends (vtable [5] 0x43920) when the buffer runs out after the first second
  (0x1a3454) or the last entry is more than 0.1 s old: the buffer closed, 0x3d890, the clock 0, the three crafts back
  on player_start0 / 1 / 2, camera0 "replay off": the run starts again from the line.
- **Replay cameras**: camera0's update (0xa3030) takes "replay on" (mode +4 = 0 on the tripod found by name, +0x478:
  "replay0output") and "replay off" (+0x3d = 1, collisions); in mode 0 it copies that tripod (eye +8, look +0x20, fov
  +0x38). The tripod is written by the replay director, CReplay (D:\neon_src\Misc\Source\replay.cpp; init 0xd5b60,
  config\camera_<t>.txt parsed by 0xd5f10, update 0xd6340; labels `tools\cmm\ghidra\label_replay.py`):
  - one camera object per camera by its type: static (vtable 0x198830), moving (0x1988a8), ship (0x198858), heli
    (0x198880); vtable +4 score(craft, lap %), +0x10 enter, +0x14 update(output, craft position, velocity, dt), +0x18
    leave; +0x58 the hold time left. Its input dataport is the followed craft (position, axes, +0x44 the lap %).
  - Each frame: the first static camera to start with (no enter); the current camera is scored, then every camera;
    the best (strictly highest, from -1000) takes over once the current one's hold is over (<= 0): leave, enter, the
    debug line "Changed to cam %d with score %.1f"; the current camera then updates the output.
  - Static: score -100 when ONLYFORWARD and the craft is behind the camera (dot of the camera's back and the
    direction to the craft > 0) or ONLYINFRAME and it is off the view axis (> -0.8), or out of the panrange window (a
    .. b; with b <= a it wraps: out only for b < p < a; p wrapped under 100); 100 while held; else 100 x (range -
    distance) / range, 0 out of range. Enter: hold = holdtime, at least 1 s; fov = its fov. Update: hold -= dt;
    DYNAMICTRACKING: the look's x / z ease 10 / s to the craft + a lead x its velocity (0.15 / 0.13 / 0.11 / 0.08 / 0.05
    by speed class), y = the craft's; else the look is its target; KERB (flag 4, 0xd7824..0xd7a57): see below;
    DYNAMICFOV: fov eases 2 / s to max(5, 90 - 0.7 x distance).
  - KERB: the wanted level w = the sum over the race manager's racers (0x596df8 of them; positions 0x596e48[i] +8,
    velocities 0x596e6c[i] +8) closer than 10 m (0x1a35a4) to the camera's eye of (10 - d) x 0.05 x |v| x 0.05
    (0x1a36e4). The level s (+0xcc, 0 on enter) then, as the code has it: w > s: s += w - 10 dt s; then w < s: s += w -
    5 dt s (0x1a3554); s < 0 -> 0, s > 0.6 (0x1a3558) -> 0.6; s < w: s += 5 dt. While s > 0 the phase p (+0xc8, 0 on
    enter) += 3 dt (0x1a351c) and the look point (+0x20) = the camera's target + (sin 15p, cos 13p, sin 10p) x s x
    0.02 (0x1a36e0 / 0x1a36dc / 0x1a35a4, 0x1a36d8): it replaces the DYNAMICTRACKING look, so a tracking KERB camera
    stops following once a racer has passed (s decays by 5 dt a frame, never quite 0 for a long while). The eye and
    the fov are not shaken. The KERB cameras' targets lie about 1 m from their eyes, so the jitter (at most 0.012 x
    sqrt 3) is about 0.7 degrees: a quick shake (5..7 Hz) while ships fly by. Nothing about the craft's speed class.
  - Moving: score 100 inside its window, else -100; never held. Enter: slide 0. Update: of 11 slides 0.003 apart
    around the last, the one whose line of sight points best at the craft, eased 4 / s; the eye on start .. end at it;
    the look eases to the craft with sine wobbles (rates sin(1.2 age) + 10 and sin(0.9 age) + 10, lead + 0.025 sin(1.5
    age)); DYNAMICFOV 90 - 0.6 per m, TIGHTFOV max(3, 30 - d) at 5 / s on the craft itself, PRESETFOV fov1 .. fov2.
  - Ship: score 5 once its rest is over (15 after it was shown, 0.01 less per scoring), else decaying 0.1 a scoring;
    enter: held 1.5 s; the eye and look at its offsets in the craft's space; fov its fov.
  - Heli: scores 0 and does nothing.
- **Time Attack ghosts** (0x429c0 / 0x42640 / 0x43460; [frontend/game-modes.md](../frontend/game-modes.md) "Time Attack"): the buffers scratch (the lap
  being recorded), fastest (the session ghost) and preloaded (the loaded ghost); one registered entry 0x1a2a0c; the
  ghost crafts dead-reckoned from the last entry (0xd06f0). Ported in argon as its own sampling (src\logic\ghost.hpp).
