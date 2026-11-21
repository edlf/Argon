# Data from default.xbe

What argon takes from the game's executable (the USA `default.xbe`). Only the HRTF tables are read from a file at run
time; every other value is written into the source, where its comment cites the XBE address it came from.

| file | what |
|---|---|
| [instructions.md](instructions.md) | values read off instructions |
| [tables.md](tables.md) | tables and constants |

## Read at run time: the HRTF tables
- 2222 Sensaura filters of 32 bytes (31 signed eight-bit taps, the interaural delay) at 0x1459d8 and the 61 x 31
  azimuth / elevation index (u16) at 0x156f98, both in the DSOUND section ([audio/mix-levels.md](../audio/mix-levels.md) "Mix levels").
- `python -I tools\assets\extract_hrtf.py [--game <folder>] [--force]` copies them into `<game>\audio\hrtf.bin`: "ARHR", u32
  version 1, u32 filter count, u32 index count, then the filters' and the index's bytes as in the XBE. Argon
  (audio\hrtf.cpp) reads that file and falls back to `default.xbe` when it is missing; tests\audio_effects_tests.cpp
  checks both give the same tables.

## Written into the source
`python -I tools\xbe\list_xbe_data.py --markdown` regenerates the tables below from the source's comments, and
`--check` compares each single float constant with the float at its address in the XBE (92 compared, 2026-11-20; the
only differences are deliberate and say so in their comments: blast_push.cpp's kSidePerPush is 0x1a372c negated,
replay_weapons.cpp's kDirectionScale is the reciprocal of 0x1a3a0c).

Kept here: a value counts when its address is cited on a `constexpr` / `const` definition or in the comment block
right above one. Addresses in the XBE's file-backed data (.rdata 0x18ecc0..0x1a54b0, .data 0x1a54c0..0x1beb58) are
tables and constants the game reads; code addresses (.text) are values read off an instruction (an immediate, a
`push`). Addresses past .data's file bytes (0x1beb58 on, e.g. 0x52afa8) are the game's run-time state, not data, and
are left out, as are mentions of an address that define nothing (function names, vtables, globals argon mirrors).
A value whose comment cites no address is not found; when adding one from the XBE, cite where it came from.
