# Models: meshes, bones, animation, morphs

## CmmMesh (200 bytes)
| off | meaning |
|---|---|
| 0x00 | owning bone index |
| 0x04 | UV set count (the vertex type's UV pairs) |
| 0x08 | vertex count |
| 0x0c | flags: 1 strip, 2 morph, 0x20 surface block, 0x80 sort point, 0x100 uses its bone record's state (hidden, clock, intensity), 0x800 weighted normals, 0x8000 drawn from a prebuilt pushbuffer (+0x44), 0x20000 hidden, 0x40000 skinned; set at load: 0x200000 pushbufferable |
| 0x10 / 0x14 | runtime: next / previous mesh on the same bone |
| 0x24 | runtime index buffer |
| 0x28 | face count |
| 0x34 | number of texture stages |
| 0x38 | s16[4] texture index per stage |
| 0x48 | s16 material index |
| 0x4c | float[3] with flag 0x80: the point the race sorts the see-through mesh by (eye distance, 0xafcf0) |
| 0xab / 0xac / 0xb0 | u8 LOD band, f32 near, f32 far (0xb7920): 0 always drawn; 1 / 2 / 3 when its bone is nearer than the model's first LOD distance / between the first and second / the second and third (model +0x5ec / +0x5f0 / +0x5f4); 4 between near and far (the power-ups: 0..600) |
| 0xc2 | s16 vertex type |

Vertex types (the D3DCOLOR is the diffuse colour the texture is multiplied by;
untextured meshes such as sky domes are coloured by it alone): `0..3` = pos f32x3, normal f32x3, D3DCOLOR, then 1..4 UV pairs
(stride 0x24/0x2c/0x34/0x3c). `4..7` = pos f32x3, **D3DCOLOR** (prelit colour + alpha, not a normal), then
1..4 UV pairs (stride 0x18/0x20/0x28/0x30); no normal (argon builds one from the triangles). Used by
the big terrain / road meshes (alpha 0xfe) and fading effect meshes such as bracken's shore waves (alpha 0..0xff).
`cmm.py` builds the normals the same way and `CMM.colours()` returns the colours (exported as glTF `COLOR_0`).

## CmmBone (0x1a0 bytes)
| off | meaning |
|---|---|
| 0x000 | local 4x4 (D3D row-vector, translation in row 3) |
| 0x040 | world 4x4 = local x parent world (0xb8630), saved as last computed |
| 0x080 | local x the parents' +0x080 (the matrix older notes call "world as saved"; spinning parts are mid-rotation) |
| 0x100 / 0x102 | s16 animation track, u16 flags (rebuilt at load: 1 hidden, 2 detached, 8 has a next sibling, 0x40 identity local, 0x80 own clock +0x188, 0x100 has children, 0x800 copies bone +0x190, 0x1000 tracks from +0x194) |
| 0x104 / 0x108 / 0x10c | runtime tint, occlusion query, tinted colour |
| 0x11c | row scale x y z |
| 0x128 | runtime: its first mesh |
| 0x12c | number of meshes on this bone |
| 0x134 | the bone's own index (mesh +0x00 refers to it) |
| 0x138 | runtime intensity (1.0 at load) |
| 0x140 | name, NUL-terminated |
| 0x180 | s16 parent, first child, next sibling, previous sibling (the children are walked first child -> +0x184) |
| 0x188 / 0x18c | runtime clock, copy of +0x100 |

Mesh vertices are in their bone's local space. world = local × parent world.

## Animation (header +0x40 / +0x44: records of 0x80 bytes)
Loaded by `cmm_fixup_pointers` (0xb6070), sampled per bone by `cmm_bone_animate` (0xb8630). Every front-end
character (`frontend\characters\<pilot>`) has them: 136..246 tracks, 20 keys a second, 103..454 keys.

| off | meaning |
|---|---|
| 0x00 | 0 in every file, not read |
| 0x04 | flags: 1 position keys, 4 rotation keys, 2 scale keys (mostly 5, then 0, 4, 6, 1; the .cms reader 0xb5270 also knows 8 = rotations stored as quaternions, which the .cmm loader does not test) |
| 0x08 | key count (shared by the three streams) |
| 0x0c | keys per second (mostly 20, 10, 5, 15) |
| 0x10 | name = the bone's name (0x40 bytes); not read: a bone finds its track by index (+0x100, 0xb8630) |
| 0x50 / 0x54 / 0x58 | runtime pointers to the position / scale / rotation keys |
| 0x5c | 0x24 bytes, 0 in every file, not read |

Survey (`record_survey.py CmmAnimationTrack`): 9042 tracks in 131 files, all the characters' and story models'.

The keys follow all the records, per record in this order: positions (12 bytes a key), rotations (16 bytes: Euler
x, y, z in degrees and a pad; the loader turns each into the quaternion of Rx · Ry · Rz in place, 0xf2ec4 /
0xf2f3e / 0xf2fb9 -> 0xf33fe), scales (12). Key 0 equals the bone's stored local matrix.

Bone fields used: +0x100 s16 track index (-1 = none), +0x102 flags (below), +0x11c..+0x124 row scales. Per bone:
frame = clock x fps; keys k0 = floor(frame) % keys, k1 = (k0 + 1) % keys (each track loops on its own count),
t = frame - floor(frame); rotation = slerp(k0, k1, t) as a matrix with row r x scale[r] (no rotation keys: the
stored local rotation); translation = lerp of the positions (else the stored one); scale keys multiply the rows.
world = local x parent world. The clock is the model's +0x5e4: the character pages set it to the seconds since the
character first showed (0x15db0). Argon (`Cmm::pose` / `Cmm::skin`) re-skins every frame.

### Bone flags at run time (+0x102, 0xb8630)
The file's +0x102 is thrown away: neon_model_load_skeleton (0xb9540) rebuilds it (1 hidden, 8 has a next sibling,
0x40 identity local, 0x100 has children). The data\characters models store 0x800 on most bones; it has no effect.
What 0xb8630 reads, and who sets it (a scan of every write to +0x102 / +0x103 in the XBE):

| flag | 0xb8630 | set by |
|---|---|---|
| 0x2 | detached: the bone keeps its own world (+0x40) and hierarchy (+0x80) matrices | the launch (0x64e20): projectiles (class 0, 0x65124) and beams (class 1, 0x65227) set it; shields (class 2) by the selector below |
| 0x40 | local = identity: world = parent world | the loader; cleared by the craft code when it moves a bone (0x51a5d..0x582a9, the flaps / jets) and by the launch |
| 0x80 | the track is sampled at the bone's clock +0x188 instead of the model's +0x5e4 | 0xb9f70 (a subtree), called by the shield launch (0x656db, class 2 only) and the shield update (0xb9e50); [weapons/shields-beams.md](../weapons/shields-beams.md) "Shields" (ported there) |
| 0x200 | after the pose, the world rotation is reset to identity (the translation stays) | the shield launch only (class 2, 0x65351..0x65766, once per view's model copy): the record's +0x194 read as an int selects the bone's mode: 0 attached (0x40, 0x2, 0x200 cleared), 1 attached and world-aligned (0x654e7), 2 detached, 3 detached and world-aligned (0x6566d), others leave the flags. Every shield record has 0 ([weapons/README.md](../weapons/README.md)), so the game never sets 0x200 |
| 0x400 | play once: keys clamped to the last one, key 0 while the frame is <= 0 | nothing |
| 0x800 | copies the local, world and hierarchy matrices of bone +0x190 (u16) | nothing |
| 0x1000 | the track comes from another model's track table +0x194 (same index) | nothing |
| 0x10 with 0x2 | aim: a look-at from the bone to the point at +0x13c, z scaled by the distance | the beam launch (0x652d5, +0x13c = the beam's end, 0x65276; also sets 0x20, 0x65327); ported as the beam's look-at frame |
| 0x2000 / 0x4000 | the world's z axis turned towards the point at +0x13c by the share +0x198 (0xb8f2e): z = unit(point - bone), y' = unit(unit(y) x z) x z, then z = unit((1 - s) z_posed + s z), y = unit((1 - s) y_posed - s y'), squared up the same way (x = unit(y x z), y = -unit(x x z)), each row times the bone's +0x11c scale; the translation stays. 0x4000 first copies the world to +0xc0 and its children hang off that (the unturned bone); with 0x2000 they hang off the turned one | the story scenes' head turns (0x99360, [sequences/scenes.md](../sequences/scenes.md) "Head turns"); ported (bone_aim.hpp) |

Copy and the beam's aim are engine features no code in this game uses (play once, foreign tracks and the head turns are
the story scenes', [sequences/README.md](../sequences/README.md)); the per-bone clock is
the shields' (ported with them); world-aligned (0x200) is never selected by the data. A projectile bone is detached without
its own clock, so its tracked children play on the ship model's clock, which the ship's draw (0x59f6f -> 0xb83e0,
0xb843f) sets to the race clock (no craft model has header flag 0x80000).

## Morph meshes (mesh flag 2)
After the mesh's vertices: a 0x10-byte header (f32 keyframes a second, u32 kind, u32 stream count, stale pointer), then
count x 8 bytes (stale stream pointer, runtime vertex buffer; the second dword holds leftover memory in the files),
then count streams of numVerts x stride: kind 1 = positions (12 bytes), 3 = + normals (0x18), 5 = positions +
D3DCOLOR (0x10), 7 = all three (0x1c), 4 = D3DCOLOR (4) (the loader 0xb6070; 0xb5e62 releases one vertex buffer a
stream). Only data\objects\explodefirelarge has them (18 meshes, kind 1, 10 keys at 3 a second), and **the game never
loads that model** (no string names it; explosions are objects\EXPLOSION_Type%d), so nothing in the game shows them.

They are drawn by the "Keyframe P - No Tex" / "Keyframe PN - No Tex" / "Keyframe PN - 1 Tex" / "Keyframe P - 2 Tex"
effects (prepare / draw 0x7ac80 / 0x7b0d0, 0x7ad40 / 0x7b2b0, 0x7ae70 / 0x7b530, 0x7afb0 / 0x7b7e0); explodefirelarge
uses "Keyframe P - 2 Tex". Its prepare builds stream 0 = D3DCOLOR, uv 0, uv 1 (0x14 bytes) and one position buffer a
keyframe. Its draw: cycle = streams / keyframes a second; phase = the fraction of (clock / cycle) with the clock at the
draw parameter's +0x3c; frame f = phase x streams; k0 = f converted by fistp (the CRT keeps round-to-nearest; `_ftol`
only truncates around its own conversions), k0 = 0 when it reaches the count; k1 = k0 + 1 (0 after the last key);
t = min(f - k0, 1), so -0.5..0.5 mostly. Streams 1 / 2 = keys k0 / k1, c5 = (t, 1 - t), and the vertex program:
position = v0 x (1 - t) + v3 x t, oD0 = vertex colour x c4 (the parameter's D3DCOLOR at +0x24), oT0 / oT1 = uv 0 / 1 +
c10 / c11 (the parameter's +0x4 / +0xc offsets). That v0 is stream 1 (k0) follows the declaration order (0x19d168,
built at run time) and is assumed. The draw parameter is the model's effect slot (Effects above: model +0x3a8 + 0x48
x record): the clock +0x3c is written only by mesh_apply_material_stages (0xba6b6, the mesh's draw time: model
+0x5e4, a bone with its own clock's +0x188, 0xb6f14 / 0xb6f2a) and only when the slot's flags have 0x100; the colour +0x24 and
the offsets +0x04..+0x10 are never written either. No code sets those flags, so in the game the clock stays at the
slot's initial value and the morph would not play; moot, since the one morphing model is never loaded.
The other three are in no file. "Keyframe P - No Tex" (0x7ac80 / 0x7b0d0): stream 0 none, one 12-byte position
buffer a keyframe. "Keyframe PN - No Tex" (0x7ad40 / 0x7b2b0): stream 0 = D3DCOLOR (4 bytes), keyframes of position
+ normal (0x18, morph kind 3). "Keyframe PN - 1 Tex" (0x7ae70 / 0x7b530): stream 0 = D3DCOLOR + uv (0xc). Their
draws do not use a clock: one global phase (0x439fc0) grows by 0.001 every time any of them draws a mesh and wraps
to 0 above 1; k0 = round(phase x streams) (0 at the count), k1 = k0 + 1 (0 after the last), t = min(phase x streams -
k0, 1); streams 1 / 2 = keys k0 / k1; c4 = 1 (white), c5 = (t, 1 - t), c6 the screen offset; the PN ones add c7 =
unit(1, 2, 1) (a fixed light), c8 = 0.7 (diffuse), c9 = 0.2 (ambient), and PN - 1 Tex c10 = 0 (uv offset). So their
speed would depend on how many such meshes are drawn a frame.
Argon: `Cmm::morph` (src\model\cmm_morph.cpp) blends the positions at the model's clock in the animated-model path (the
game would show key 0 frozen; nothing loads the model).
