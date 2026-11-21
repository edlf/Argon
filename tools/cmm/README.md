# cmm tools — Quantum Redshift models to Blender

No dependencies beyond Python 3.

| file | what |
|---|---|
| `cmm.py` | reader for `model.cmm` (`python -I cmm.py <model.cmm>` prints a summary) |
| `xdds.py` | decodes the game's Xbox `.dds` textures to RGBA / PNG |
| `cmm2gltf.py` | one model folder → `.glb` (meshes, bone hierarchy, textures) |
| `gltf2cmm.py` | the way back: an edited `.glb` → `model.cmm` (the meshes' geometry; every other byte kept) |
| `export_all.py` | ships, menu ships, tracks, skies → `export/craft`, `export/feship`, `export/tracks`, `export/tracks_sky` (`python -I export_all.py tracks` for one group) |
| `trackdata.py` | read/write start grid, sprites, fog/light, AI nodes, pickups, cameras |
| `blender/track_tools.py` | Blender add-on: load a track + markers, write marker and mesh edits back |
| `blender/render_preview.py` | headless Blender import + preview render (used for testing) |
| `blender/test_*.py` | headless Blender tests of the add-on (`blender -b --factory-startup -P blender\test_track_tools.py -- <track> <out_dir>`) |
| `effect_survey.py` | which meshes use each effect record, the texture tables' types / frames, the unread stage words (`python -I effect_survey.py [game_data]`) |
| `record_survey.py` | every binary record type's dwords over the disc: how many are non-zero, their values (`python -I record_survey.py CmmEffect`) |
| `xpu.py` | a pixel shader (`.xpu`) as readable register-combiner equations |
| `xvu.py` | a compiled vertex shader (`.xvu`) as D3D-style assembly |
| `disasm.py` | disassembles `default.xbe` at a Ghidra address (Capstone) |
| `xbe_strrefs.py` | the strings in `default.xbe` matching a regex, with the code that references them |
| `ghidra/` | Jython scripts used on the `neon_stock` Ghidra project |

Track files, collision and what can be edited: [docs\tracks\README.md](../../docs/tracks/README.md); every format and
decoding note: [docs\README.md](../../docs/README.md).

```
python -I cmm2gltf.py ..\..\game_data\data\craft\abi export\abi.glb
python -I export_all.py
```

In Blender: File → Import → glTF 2.0 → pick the `.glb`. Each part is an
object named after its game bone (e.g. `AIRBRAKE_LEFT`, `CANOPY`, `JET_`).
Effect meshes (shields `*SHIELD*`, `SPECIAL*`/`revol*` weapon fx, `JFLARE*`
glows, `JET_*` exhaust, `STAR*`, `SHURIKEN*`) sit on top of the hull — hide them
to see the ship.

## Writing a model back

```
python -I gltf2cmm.py ..\..\game_data\data\craft\abi\model.cmm abi_edited.glb abi_model.cmm
```

Edit the meshes in Edit Mode (vertices, faces, UVs, vertex colours), then File → Export → glTF 2.0 with
Include → **Custom Properties** ticked, so each mesh keeps its `qr_mesh` number (else its name `meshNN` is used).
Each mesh is written in its original vertex type, as a triangle list; everything else in the file stays as it was.
Not written: skinned and morph meshes, added or deleted meshes, bone and object placement (move vertices, not objects).
A mesh holds at most 65535 vertices.

For tracks the add-on does it in one click: **Write track model to game files** writes the track's and the sky's
mesh edits into their `model.cmm` (a `.orig` copy is kept the first time). Headless test:
`blender -b --factory-startup -P blender\test_model_write.py -- <track> <out_dir>`.

