# Tracks

18 tracks (9 normal + 9 `alt_`). Data is split between the track folder and
the game's `config\` folder (plain text).

## Where everything lives
| file | what | tool support |
|---|---|---|
| `data\tracks\<t>\model.cmm` + `tex*.dds` | whole track geometry **and collision** | export to .glb (`cmm2gltf.py`); writing back not built |
| `data\tracks\<t>\sky\model.cmm` | sky dome | export to .glb |
| `data\tracks\<t>\locator.dat` | start grid (`player_start0..8`) | read/write, Blender |
| `data\tracks\<t>\sprite.dat` | glow/flare quad sprites | read/write position, Blender |
| `data\tracks\<t>\prop.dat` | fog, ambient + sun light, track name | read/write, Blender custom props |
| `config\ai_<t>.txt` | AI node graph: racing line, track edges, corner/turbo hints | read/write, Blender |
| `config\<t>pup.txt` | pickups (`POWERCHIP`, `MEGAUP`, `POWERUPRED/BLUE/YELLOW`); the game lifts each by a per-type offset (3.1–6.0, see [audio/README.md](../audio/README.md)) | read/write, Blender |
| `config\camera_<t>.txt` | replay/attract cameras (pos, target, fov, flags) | read/write pos/target, Blender |
| `config\ai_values_<t>.txt`, `aivalues_<pilot>.txt` | AI difficulty tuning (tab-separated tables) | edit in a text editor |
| `config\trackgoals.txt`, `setup.txt`, `cashmodifiers.txt`, `character_stats.txt` | game rules / stats | text editor |
| `data\tracks\<t>\water.dat` | water areas: u32 count + 0x1c header, then count × 40 bytes (u32, u32, 0, 0, min xyz, max xyz). Surface = box top; the game draws it procedurally with `envdata\waterref1/2.dds` cube maps | decoded (argon draws it) |
| `data\tracks\<t>\visdata.vis` | visibility sectors: which meshes the race draws from where (below; none on Mars) | decoded, used by argon in a race |
| `data\tracks\<t>\route.dat` | 512 x (x,y,z,0) line — **not loaded by the game** (no reference in the XBE; AI uses ai_<t>.txt) | ignore |

All coordinates are game space: right-handed in effect, Y up (see [models/README.md](../models/README.md)).
Blender (after the glTF import) = game `(x, -z, y)`.

## Binary formats (confirmed in Ghidra, project `neon_stock`)
**visdata.vis** (loaded by the track loader at 0xafa66 unless the model's header has flag 0x10; read by the race's
track draw 0xafcf0 through 0xbede0): u32 sector count, a stale pointer, 16 unused bytes (zeros or leftovers); then
count x 0x1c: f32 x, y, z, u32 neighbour count, u32 mesh count, two stale list pointers; then per sector its mesh
indices (u32, into model.cmm's meshes) followed by its neighbour sector indices (u32). The sizes add up exactly on all
16 files (28..63 sectors) and every index is in range.
In a race 0xafcf0 picks the sector nearest the eye (3D distance to x, y, z; the first of equals), marks the meshes of
that sector and of each of its neighbours (not their neighbours) and draws only those: the others are culled against
the view (a sphere around the vertex bounds) and drawn or sorted as before; meshes with mesh flag 0x100 (on a bone) are
left to 0xb8500, which draws them only when marked too. Debug value 2 in 0x439e40 marks every mesh. Mars has no file
and flag 0x10, so it draws everything. Argon: src\track\track_visibility.cpp; SceneRecorder hides the unmarked track
meshes while a race or drive runs (RaceSession::driving; the eye = the drawing camera, also a free one), else draws everything.

**locator.dat** (`locator_dat_fixup` 0x000af270): u32 count, stale ptr, 0, 0;
then `count` x 0xc0: `name[64]`, world 4x4 @+0x40 (D3D row-vector),
u32 @+0x80, `pos[3]` @+0x84, `rot[3]` degrees @+0x90 (matrix = Rx·Ry·Rz),
`scale[3]` @+0x9c. The matrix must match pos/rot/scale — `trackdata.py` rebuilds it.

**sprite.dat** (`sprite_dat_fixup` 0x000af320 → `quadsprite_AddCMMSprites` 0x000bdf40):
u32 count + 0x14 header; `count` x 0x84: s16 facing @+0 (x 3 = the vertex shader's a0: 0 world, 1 upright, 2 camera;
the files use 1 for trees, 2 for clouds / glows), u32 flags @+4 (bit0 additive; bit 1 is set in many files but not
read), `pos[3]` @+8, corner offsets @+0x14/+0x20/+0x2c/+0x38 (= v3, v2, v0, v1), 4 corner colours (D3DCOLOR, v0..v3)
@+0x44, UVs @+0x54..+0x70 (v0 u +0x54 v +0x70, v1 +0x5c / +0x68, v2 +0x6c / +0x58, v3 +0x64 / +0x60; the game uses
v x 0.5 + 0.5: the lower half of envdata\quadsprite.dds, the track's own trees / clouds); +0x74..+0x84 unused.
Counts: Bracken 32, alt_bracken 98, Mount 348, alt_mount 439, Silver / alt_silver 9, the others none. Drawing:
[effects/sprites.md](../effects/sprites.md) "Track sprites"; argon: src\track\track_sprites.cpp.

**prop.dat** (`track_load_prop_dat` 0x000b03f0): 4096-byte files, of which the game copies 0x1178 bytes to the
global 0x390fa8. Only the fog, ambient, sun colour and sun vector fields have readers in the XBE; the others
(including +0x10 below and +0x1c / +0x20 / +0x124..+0x134 = 1 or 0) are never read. Structs:
`src\original\track_records.hpp`.
| off | field | evidence |
|---|---|---|
| 0x000 | fog colour r,g,b (int 0–255) | packed to 0xffRRGGBB for fog |
| 0x00c | fog density (float) | passed to `neon_setup_fog` |
| 0x010 | colour r,g,b | reset to 0x40 with the ambient (0xaf360); not read |
| 0x024 | track name char[64] | |
| 0x13c / 0x140 / 0x144 / 0x148 | int, int (500–40000), int (60/70), float (the same values as water.dat's wave amplitude) | not read by the game |
| 0x14c | ambient light r,g,b | blended into ship lighting |
| 0x158 | sun colour r,g,b | default 0xcc |
| 0x164 | sun vector x,y,z | normalised by the game; default (-0.5,-1,-1) |
| 0x170 | fog mode (int) | |
| 0x174 | fog range (float) | passed to `neon_setup_fog` |

## AI file (`config\ai_<t>.txt`, loaded by `AI::LoadAndInitialise` 0x000e5610)
- `<nodes>`: `pos`, `down` (surface up-vector), `forward*`/`backward*` links,
  flags (`flag_first`, `flag_restart` = respawn allowed, `corner_slow/hairpin/straight`,
  `turbo_good_*`/`turbo_never_*` per speed class, `env_inside` (node flag 0x80: under a roof, no rain sound;
  [effects/rain.md](../effects/rain.md) "Rain"), `reverb_*`).
- `<hulls>` (one per node): `transplane = (half vector to neighbour node) + neighbour id`
  — verified exact on bracken; `trackdata.write_ai` recomputes these when a node moves.
  `boundaryplane` = left/right track edge offsets relative to the node (kept as-is).
- `<routes>`: total `length` and branch info. Not recomputed — keep big changes small,
  or adjust lengths by hand.

## Collision comes from the track mesh
`collision_build_from_cmm` (0x000d29c0): a mesh is solid when it has a surface
block (mesh flag 0x20) whose `+0x0c & 2`. The .glb groups static meshes into
`world_collision`, `world_invisible_walls` (solid, untextured), `world_visual`.

## Editing in Blender
1. Edit → Preferences → Add-ons → Install from Disk → `tools\cmm\blender\track_tools.py`
   (install from that folder location; it finds the game from there).
2. 3D view sidebar (N) → **QR** tab → pick track → **Import track + markers**.
3. Move objects in the `QR starts` / `QR AI nodes` / `QR pickups` / `QR cameras` /
   `QR sprites` collections; rotate start arrows; edit fog/light under
   `QR_track_settings` → Object Properties → Custom Properties.
4. **Write markers to game files**. First write keeps `<file>.orig` next to each file.
   Track geometry is *not* written (needs the .cmm writer).

Tests: `python -I trackdata.py <track>` (byte-identical round-trip of every file),
`blender -b --factory-startup -P blender/test_track_tools.py -- <track> <out_dir>`,
`blender -b --factory-startup -P blender/test_alignment.py -- <track>`.
