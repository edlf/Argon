# Replays (the event buffer)

The game records every race into an event buffer (CEventBuffer, `D:\Neon_Src\Misc\source\eventbuffer.cpp`) and plays
it back for View Replay (the race end's menus), the pause menu's ghost replays, Time Attack's ghosts and the front
end's attract demo. The attract demo's three races are on the disc (`replays\replay1..3`, `game_data\replays`).
Reader: `tools\saves\replay.py` (Python, prints the header, the first entries and totals) and `src\logic\replay.{hpp,cpp}`
(C++, tests in `tests\replay_tests.cpp` on the three files).

| file | what |
|---|---|
| [argon.md](argon.md) | argon's port |
| [users.md](users.md) | who uses the event buffer |

## Files
A buffer is `<name>.index` + `<name>.events` (the attract demo adds `<name>.info`). The game keeps its buffers on the
Xbox's cache drive: `z:\eventbuffer\scratch`, `fastest`, `record`, `preloaded` (Time Attack; created by
neon_open_eventbuffer_files 0xda6c0) and `z:\replay<n>` (the attract demo, copied from `d:\replays` by attract_init
0x40440 with CASyncFile::Copy 0xd8d00). A CReplay object (neon_init_something 0xd9b70: "demo", "replay", "scratch",
"fastest", "preloaded") holds an index buffer and an events buffer in memory (mode +0 = 1) or streams the files
(CASyncStream); the disc's files are 18432 index entries (147456 bytes) and 6.0 .. 6.5 MB of events, zero padded.

- `.info` (text, fe_shell_start_attract 0x31c10): `track <name>` (silver 0, kalau 1, fuusoku 2, flames 3, solar 4,
  bracken 5, mount 6, draco 7, alt_silver .. alt_draco 8..15, mars 16, alt_mars 17) and `craft<i> <pilot>` (tara 0,
  yuri 1, kumiko 2, darius 3, jordan 4, marcus 5, leah 6, kukai 7, alicia 8, vladimir 9, abi 10, zafir 11, kaitlyn
  12, jaya 13, lukas 14, kutoo 15). replay1 bracken (marcus, jaya, kumiko, kukai, jordan, zafir), replay2 silver
  (tara, alicia, lukas, jaya, zafir, kukai), replay3 flames (darius, zafir, tara, abi, kumiko, jordan).
- `.index`: 8-byte entries {float time, u32 offset into .events}, one per recorded frame; the first time -1 ends it
  (the disc's: 10736 / 11153 / 10901 frames, 214.74 / 223.13 / 218.06 s; a frame every 0.02 s on average, 0.011 ..
  0.1 s).
- `.events`: per index entry a chunk at its offset: a 16-byte header {u32 size (header included), float time (= the
  index time), u32 continuous entries, u32 critical entries} and the entries back to back. Each entry starts with the
  same 0x18 bytes: +0 the entry object's vtable (0x1b6990 for a craft; not read back), +4 type (1 continuous, 2
  critical), +8 size (< 0x1000; 0x58 a craft, 0x40 a weapon event), +0xc the target's name hash, +0x10 a byte "new"
  (set to 1 on reading), +0x14 float time.
- Name hash (obj_send_message 0xb4e00): h = 0; for each whole dword at byte i: h ^= dword + i; then for the k-th of
  the remaining bytes from the end: h ^= (signed char) + k. "craft0" = 0x66617226 (the bytes `&raf` in the files).
  Unused bytes are 0xfc / 0xbd / 0xc2 (uninitialised memory of the recording build).

## Craft entry (type 1, 0x58 bytes, registered as "craft%d")
The craft's entry object is embedded at craft +0x7e4 (so +0x10 is craft +0x7f4, +0x14 craft +0x7f8, ...).
| entry | craft | what |
|---|---|---|
| +0x18 | +0x7fc | position (craft +0x27c) |
| +0x24 | +0x808 | velocity, m/s: the previous state block's +0x3c (equal to this step's displacement / dt) |
| +0x30 | +0x814 | angular momentum L (the current state block's +0x54) |
| +0x3c | +0x820 | rotation rows 0 and 1 (right, up of the row-major matrix; columns are right, up, forward), each element x 4096 as a short in the low half of a dword (0x582d0); row 2 = row 0 x row 1 on reading (0x423b0, x 1/4096) |
| +0x54 | +0x838 | control bits |

Control bits (craft_record_inputs 0x61aa0; craft_replay_inputs 0x61c60 writes the slots back as 0 / 100): 1 throttle
[0] > 50, 2 fire (+0x91f), 4 [5] steer, 8 [6] steer, 0x10 [2] dive, 0x20 [3] climb, 0x40 [9] brake, 0x80 [7] airbrake,
0x100 [8] airbrake (slots as [physics/README.md](../physics/README.md); the others > 0).

## Weapon entry (type 2, 0x40 bytes, "WeaponEvents")
Written through CEventBuffer::WriteCriticalEntry 0xdaa60 from one static entry (0x1be930) by 0x641a0 (a signal),
0x646e0 / 0x64870 (projectile states), 0x649f0 (signals with data) and weapon_launch 0x659eb, only while recording
(0x52afa0), not in Time Attack (mode 3) or a replay (0x52afac). Played by its callback craft_weapon 0x641f0
(RegisterCritical "WeaponEvents" / "WeaponCallback", by neon_gamelogic_record_replay 0x3f210 and attract_init).
- +0x18 kind: 1 a projectile's state, 2 a signal. +0x1c projectile, +0x1d weapon slot, +0x1e craft, +0x1f a byte (the
  launch's target), +0x20 a running count (0x439fc8).
- Kind 1: +0x24 position, +0x30 velocity, +0x3c..+0x3f bytes (homing: the lock, a colour x 0x10101; else a direction
  x 127 as signed bytes, read back x 1/127 0x1a3a0c).
- Kind 1's direction bytes are the negated forward: (-90, -4, 88) for a velocity (212.5, 10.8, -207.1) in replay1.
- Kind 2, +0x24 signal, as craft_weapon 0x641f0 plays them (its switch through 0x646a8; the craft +0x1e, its weapon
  slot object +0xc0 + slot +0x1d x 0x1730):
  | signal | played by | what |
  |---|---|---|
  | 1 | 0x6ac90 | the craft's shield (weapon +0) = the float +0x2c; at 0 or below (the craft not dead) knocked out (0x6a8a0) with +0x1f the killer. The replays' damage: the hits themselves are not replayed |
  | 2 | 0x64e20 | projectile +0x1c launched at the entry's time, +0x1f its locked target (+0x370 + 4 x projectile) |
  | 3 | 0x65e60 | the projectile is done |
  | 4 | 0x65f80 | the impact as shown: +0x28 the kind (1 craft, 2 world, 3 expired, 4 bounce), +0x2c where (its bursts) |
  | 5 | - | nothing |
  | 6 | | the craft weapon's +0x9c = +0x28, then 0x6b950 for each player (below) |
  | 7 | 0x6a390 | the weapon slot +0x28 fires (a shield goes up; the projectiles follow as launches) |
  | 8 | | pickup +0x2c taken by craft +0x30: gone (0x52b0b4 + 0x60 x pickup = 0), a burst on the craft by its type |
  | 9 | | pickup +0x2c back |
  | 0xb | 0x6a6c0 | the shield's retaliation, +0x1f the attacker |
  | 0xc | 0x64b30 | then as 4 |
- Signal 6 is written by weapon_pickup 0x6ba30 (0x6bc64, for every craft, after the "PICKUP" debug line) with the
  weapon's +0x9c: weapon_select's slot (0x6c110 sets it to the fired slot and back to -1 after the shot; the knock-out
  0x6a7cb also -1), which a pickup raises by 1 (a mega-up by 5) up to the record count - 1. So +0x28 counts the
  pickups since the last shot, from 0: replay1's craft 0 writes 0 / 1 / 2 between its fires. It is not the power-up
  levels (+0xa4 / +0xa8 / +0xac / +0xb0, which no replay entry carries): played back it only sets +0x9c (the next
  fire overwrites it; 0x6bd70 copies it to huddata +0x90, which no HUD code was found to read) and calls 0x6b950,
  the SHIELD bone flash that no craft model has. Nothing visible; not written or played here.
- The disc's counts (replay1 / 2 / 3): 3947 / 3400 / 5492 weapon entries; signals 1: 143 / 159 / 183, launch 113 /
  100 / 187, done 112 / 100 / 233, impact 398 / 361 / 645, 6: 238 / 172 / 188, fire all 57 / 59 / 60, pickup taken
  241 / 172 / 189, respawn 237 / 172 / 188, 0xc 56 / 60 / 60, 0xb 0 / 2 / 0. Kind 1: 2352 in replay1 (each projectile
  every frame it flies).

## Recording (CEventBuffer write mode, eventbuffer mode +8 = 1)
- neon_gamelogic_record_replay 0x3f210 (game logic vtable +0x18, from race_clock_reset 0x3f1d0 at GO): clears the
  buffer (0xd9e50, +0xd2d4), registers "craft0".."craft<n-1>" (CEventBuffer::Register 0xda7c0, up to 128 entries) and
  "WeaponEvents"; +0xd2d8 = recording. Stop: 0x3f2f0 (vtable +0x1c), called where POSTABLE opens (0x52afa8 = 5:
  0x3baa8, 0x3fc84, 0x3fd9c; the player's finish, the time limit, the knock-out), so a race's recording ends with the
  player's race.
- Each frame (0x3eee0, from the race update): in Tournament / Quick Race the recording stops past 1200 s (0x1b9790;
  the 20-minute limit). At most every 0.015 s (0x1a3958) after the last record time +0xd2c0, for each craft (skipped
  while 0x52afac, a replay plays): the dead-reckoned position from its last entry (pos + vel x (now - time)) is compared
  with the craft's; an entry is written (craft_record_inputs, CEventBuffer::WriteEntry 0xda940) when that error is
  over 0.1 (0x1a3508), the velocity changed by more than 1.0, or any element of the rotation moved more than 0.05
  (0x1a3654 / 0x1a36e4) from the last entry's (+0x1044). In the disc's replays nearly every craft is written every frame.
- Then eventbuffer_advance 0xdb260 (mode 1: 0xdab80) closes the frame's chunk: the index entry {the chunk's time, its
  offset} and the chunk are written, and a new chunk starts with the new time. So a chunk carries the time of the frame
  before its entries' own (the entries' +0x14 is one frame later, e.g. 0.0 / 0.0675 in replay1).

## Playback (read mode 0: 0xdad40 / CEventBuffer::ReadEventEntries 0xdaea0 / 0xdb090)
- eventbuffer_advance(t) with t the race clock (whole seconds, int 0x50e430, + the fraction, float 0x50e434): reads the
  pair of index entries at the current position {t0, off0, t1, off1}; the replay has ended (returns 2) when t1 is -1
  or t1 < t0, or the index runs out. When t1 <= t the position moves on one entry. The chunk at off0 is read if not
  read yet (+0x128 the last read offset); up to 10 entries a call. Returns 0 while playing.
- Reading a chunk (0xdb090): each continuous entry is copied whole into the object registered with its name hash (the
  craft's +0x7e4), its +0x10 = 1 and its time = the index entry's time; each critical entry is copied (from +4) into
  the registered entry and handed to the callback.
- The crafts (craft_replay_inputs 0x61c60, each frame after the advance): on a new entry (+0x7f4) the state is taken
  (+0x1020.. position, velocity, L; +0x1044 the rotation from the shorts) and +0x7e0 = 1 (has an entry; until then
  +0x1145, dead, is set). In the first 3 s (0x1a351c) of the clock outside Time Attack the position eases 10 % a frame
  (0x1a3508) to the entry's; else 0xd06f0 places the craft by dead reckoning: x = pos + vel dt, R = R0 + dt [w]x R0
  with w = R0 Ib^-1 R0^T L (the craft's body inverse inertia +0x148), elements clamped to -1 .. 1, both state blocks
  alike. The control slots come from the bits (0 / 100; fire +0x91f), and +0x990 (the forward speed), +0xde4.. are set
  from the state as the physics would.
- The weapons replay from their entries (launches, projectile states, impacts, pickups), not from the fire bits.
