# Quantum Redshift documentation

Everything decoded from `default.xbe` and the game data so far, and the state of argon (the reimplementation).
Ghidra labels and comments: scripts in `tools\cmm\ghidra\label_*.py`.
Each folder's README.md is its overview and lists the folder's files.

## Argon

| folder | what |
|---|---|
| [argon](argon/README.md) | building, the editor, the source layout, TODO, done, deliberate differences |

## File formats

| folder | what |
|---|---|
| [models](models/README.md) | `.cmm` models: header, meshes, bones, animation, morphs, surface blocks (render state), materials, effects, textures |
| [sequences](sequences/README.md) | `.nms` scripted sequences: layout, playback, the story scenes |
| [tracks](tracks/README.md) | track files (locator / sprite / prop / water .dat, AI, pickups, cameras), collision, editing in Blender |
| [saves](saves/README.md) | the game's profiles on the Xbox HDD, their signature, `tools\saves\profiles.py` |
| [xbe-data](xbe-data/README.md) | the data argon takes from default.xbe: the HRTF tables, every value written into the source |

## The game's systems

| folder | what |
|---|---|
| [physics](physics/README.md) | craft physics, the AI pilot, turbo, cameras, the in-race HUD, opponents, standings, rumble |
| [weapons](weapons/README.md) | weapon.nwp records, firing, projectiles, hits, explosions, shields, beams, the AI's firing |
| [effects](effects/README.md) | ship shadows, trails, jets, the water wake, sprites, flares, pickups, rain |
| [water](water/README.md) | water rendering: patches, wave frames, vertex and pixel shaders, physics |
| [audio](audio/README.md) | track / ship / front-end sounds, music, race sounds, reverb, mix levels |
| [replays](replays/README.md) | the event buffer: recording, View Replay, ghosts, the attract demo, the replay cameras |
| [frontend](frontend/README.md) | the front-end shell and every page, game modes, the race's menus and race end, multiplayer |

The binary layouts are also C++ structs, byte for byte with offset asserts, in `src\original\` (`cmm_records.hpp`,
`track_records.hpp`, `weapon_records.hpp`, `nms_records.hpp`); argon's loaders read through them.

Tool usage stays next to the tools: [tools\README.md](../tools/README.md) (every folder) and
[tools\cmm\README.md](../tools/cmm/README.md) (model / track export).
