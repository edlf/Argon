# Tools

Python 3 scripts (no dependencies unless noted) for the game's files, its saves and live checks in xemu. Run them from
the repo root with `python -I tools\<folder>\<script>.py`; each script's docstring has its full usage.
Tests: `tools\tests` (run by `test.bat` through CTest, or `python -m unittest discover -s tools\tests`).

## assets — the game's files for argon
| file | what |
|---|---|
| `convert_assets.py` | converts the videos (`.ogv`), music (`.flac`) and sound effects (`.ogg`) next to the originals (needs ffmpeg; [docs\argon\build.md](../docs/argon/build.md)) |
| `delete_converted_originals.py` | verifies those conversions and deletes the originals they replace (`--delete`) |
| `extract_hrtf.py` | copies the HRTF tables out of `default.xbe` into `game_data\audio\hrtf.bin` |
| `convert_kemar.py` | converts the MIT KEMAR full set into `hrtf_kemar.bin` at the project root (Argon Options' HRTF; 512 taps, cut by KEMAR Taps; prints each cut's level and tail; needs numpy) |

## saves — profiles and replays
| file | what |
|---|---|
| `profiles.py` | reads, edits, signs and converts the game's profile saves to / from argon's JSON ([docs\saves\tools.md](../docs/saves/tools.md)) |
| `profile_editor.py` | a Tk editor for those profiles |
| `signature_key.py` | reads the saves' signing key from a running xemu into `keys.json` |
| `replay.py` | prints a replay's header, first events and totals ([docs\replays\README.md](../docs/replays/README.md)) |

## xemu — live checks in the running game
Set the `XEMU` environment variable to the full path of `xemu.exe`, then start xemu with `run_xemu.ps1 -Iso <game.iso>`
(GDB stub on tcp 1234; `-Wait` halts at boot).
| file | what |
|---|---|
| `run_xemu.ps1` | starts `%XEMU%` on an ISO |
| `xdvdfs.py` | finds a file in an Xbox ISO |
| `qrdbg.py` | the GDB client the probes use; also a command line (read / write memory, breakpoints) |
| `watch.py` | write watchpoint on a guest address |
| `tsslog.py` | logs the texture stage combine calls with the mesh draws |
| `combiners.py` | decodes the register combiners one mesh draw sets |
| `ffprobe.py` | the fixed-function D3D state of one mesh draw |
| `bumpprobe.py` | the road bump effect's shader constants |
| `craft_drawstate.py` | diffs the D3D state of two crafts' mesh draws |
| `ghost_probe.py` | compares the Time Attack ghosts with the player's craft |

## xbe — the game's executable
| file | what |
|---|---|
| `xbe.py` | XBE helpers (address ↔ file offset, hexdump), shared by the other folders |
| `list_xbe_data.py` | lists the data argon takes from `default.xbe` ([docs\xbe-data\README.md](../docs/xbe-data/README.md)) |

## viewer — argon itself
| file | what |
|---|---|
| `shot_compare.py` | refactor check: renders the same scenes with two argon builds and compares them (needs Pillow) |

## cmm — models, tracks, shaders, Ghidra
See [cmm\README.md](cmm/README.md).
