# Quantum Redshift `.cmm` model format (version 300)

Found by reading `cmm_fixup_pointers` (XBE `0x000b6070`, called after
`cmm_model_load` `0x000b5c40` opens `data\%s\model.cmm`). Ghidra project
`neon_stock` has these names plus struct types under `/QR`.

C++ structs of every record, byte for byte: `src\original\cmm_records.hpp`; `tools\cmm\record_survey.py` lists
every field's values over the disc, `tools\cmm\effect_survey.py` the meshes of every effect, the texture tables and the
unread material words.

The file is a **raw dump of in-memory structs**. Pointer fields contain stale
addresses from the tool that wrote them; the game ignores them and rebuilds
every pointer by walking the data in a fixed order. All values little-endian.

| file | what |
|---|---|
| [effects.md](effects.md) | effect records |
| [materials.md](materials.md) | materials and the texture table |
| [megaup-cage.md](megaup-cage.md) | the mega-up's cage |
| [meshes.md](meshes.md) | meshes, bones, animation, morphs |
| [surface-block.md](surface-block.md) | the surface block (render state) |

## Header (0x48 bytes)
| off | type | meaning |
|---|---|---|
| 0x00 | u32 | version = 300 |
| 0x04 | u32 | num_meshes |
| 0x08 | u32 | num_textures (`tex0..N-1.dds`, `lotexN.dds` for low detail) |
| 0x0c | u32 | flags (0x1000 = texture table present; 0x10 = no visibility file: the track loader 0xafa58 skips data\%s\visdata.vis, set by Mars only, the one track without one) |
| 0x18/0x1c | u32/ptr | num_materials, materials |
| 0x20/0x24 | u32/ptr | num_bones, bones |
| 0x28 | i32 | root bone (a bone without parent; -1 without bones) |
| 0x2c | ptr | texture table (only used as "non-zero") |
| 0x30/0x34 | u32/ptr | count/ptr of 0x5bc-byte **effect records** (see Effects) |
| 0x40/0x44 | u32/ptr | count/ptr of the 0x80-byte animation tracks (see Animation; copied to model +0x600 / +0x5fc) |

## Data order after the header
1. `CmmMesh[num_meshes]` (200 bytes each)
2. `CmmMaterial[num_materials]` (0xec each)
3. align to 16, `CmmBone[num_bones]` (0x1a0 each)
4. for each mesh with flag 0x20: pushbuffer block, first u32 = block size
5. for each mesh: vertices (if not skinned), then morph streams (flag 2), then skinned
   vertex block (flag 0x40000: 0x38 header, stride 0x38 + (bones-2)*0x10)
6. for each mesh: indices — strip (flag 1): `faces + 2` u16; list: `faces * 3` u16
7. texture table: `num_textures * 0x10` (if header flag 0x1000)
8. effect records (0x5bc each), then their vertex shader programs (size at record +0x50), then the animation
   track records (0x80 each), then their keys (see Animation)

## Textures
Normal 128-byte DDS header, but the FourCC slot holds an **Xbox D3DFORMAT**:
0x0C DXT1, 0x0E DXT3, 0x0F DXT5; or a 16-bit RGB mask format (A1R5G5B5,
used for normal maps). Data on disc is **linear** (not swizzled).

## Coordinate system
**Use the data as right-handed, Y up — no axis flip.** The game renders with a
mirrored projection (the cull helper `rs_cull_default` swaps CCW/CW when the
projection's x scale is negative), so its data is right-handed in effect:
`AIRBRAKE_LEFT` sits at +x with the nose at +z, and text/ads only read correctly
unmirrored. Front faces are **clockwise**. (Tools before 2026-11-10 mirrored Z,
which mirrored everything; fixed in cmm2gltf, the Blender add-on and argon.)

## Not done yet
- Skinned meshes (0x40000; vtype = bone count + 6, 2..8): header 0x38 (+0x04 bone count, +0x08 the bone indices (8 slots), +0x28 vertex pointer, +0x2c runtime vertex buffer),
  then per vertex n x position (each in its bone's space), normal, D3DCOLOR, uv, n x weight (stride 0x38 +
  (n - 2) x 0x10). In the rest pose (bone world = local x parent chain) the n positions agree to within a few
  tenths: vertex = sum w_k (W_k pos_k). The normal: with mesh flag 0x800 (bodies) sum w_k (W_k normal); without it
  (the faces) it is in the mesh's own bone space, W_mesh-bone normal (checked against the triangles: 0.97..0.998
  vs 0.63..0.77 when blended through the jaw bone, which is turned ~90 deg at rest). Argon skins every frame with the animation.
  Morph meshes: decoded and drawn by argon (above), not exported by the tools.
- The effect, animation-track, material and texture-table records are decoded field by field (above), Spheremap
  Decal's inputs included.
- Writing `.cmm` back: `tools\cmm\gltf2cmm.py` writes the geometry of the meshes a `.glb` holds (vertices and
  triangles in each mesh's own vertex type, as a triangle list) and keeps every other byte; it relies on the order
  above (stale pointers, nothing aligned after the bones), so a mesh's blocks may change size. Every version-300
  model on the disc rebuilds byte for byte (`tools\tests\test_gltf2cmm.py`). Not written: skinned and morph
  meshes, new or removed meshes (a track's visdata.vis and the render lists refer to the meshes by index), bones.
