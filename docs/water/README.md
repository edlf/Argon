# Quantum Redshift water rendering (from default.xbe, `Framework\Source\water.cpp`)

Disassemble the vertex shaders with `python -I tools\cmm\xvu.py shaders\waterbumpfresnal.xvu`.
Ghidra labels: `tools\cmm\ghidra\label_water.py`.

## Files
- `shaders\waterbumpfresnal.xvu`: the vertex shader used in game (handle 0x42c4d0).
  `waterbump.xvu` is created too (0x42b4bc) but never drawn with; `water.xvu` is not loaded at all.
- `data\testdata\waterbump000..059.dds`: 60 animated bump (normal) map frames, loaded by 0xcd940 into 0x447b68 (0x28 each).
- `data\<t>\envdata\default.dds`, `waterref1.dds`, `waterref2.dds` (custom1..5 too): cube maps (stages 2 / 3), per
  patch from the slot table at 0x50f880 (below).
- `data\<t>\water.dat`: the water patches (loaded at 0xcde12).

## Draw (water_draw 0xce1c0)
Per frame, with `f` = game frames (time x 60):
- stage 0 texture = bump frame `(f / 2) % 60` (30 fps, 2 s loop), wrap.
- vertex shader = waterbumpfresnal, pixel shader 0x2907ec (below).
- geometry blends two of 30 precomputed wave frames ("Water Basis Frames" 0x42b4c8, 16 bytes each):
  frame a = `(f / 8) % 30`, frame b = a + 1 (wraps), weight `w = (f & 7) * 0.125`.
  Streams: 0 = frame a +8 (stride 0x2c), 1 = frame b +8 (0x2c), 2 = frame a +0xc (12), 3 = frame b +0xc (12).
- vertex declaration 0x1a8528: stream 0 v0 NORMSHORT4 position, v1..v3 FLOAT3 basis; stream 1 v4..v7 the same
  for frame b; stream 2 v8 FLOAT3 normal; stream 3 v9 FLOAT3.
- for each patch (count 0x42b4c0, 16 bytes from 0x42b4d0: index, x, y, z): visible if the sphere at
  (x+64, y, z+64) radius 104 is in view; stages 2 / 3 = its cube maps; c5 = (x-4, y, z-4, 1);
  DrawIndexedVertices(TRIANGLELIST, 0x600) = 512 triangles = 16 x 16 quads.

Vertex shader constants (SetVertexShaderConstant 0x109800, register = raw - 96):
| reg | value |
|---|---|
| c0-c3 | world-view-projection |
| c4 | (262136, 3, 262136, 1): NORMSHORT position scale -> 8 m per grid step, +-3 m height |
| c5 | patch offset (x-4, y, z-4, 1) |
| c6 | viewport offset (screen x / y + 0.53125) |
| c7 | (1, 1, 1, 1) |
| c8 | (w, 1-w, 1, 1): frame blend |
| c9 | (0.5, 0.7, 1.0, 0.6): water colour rgb, base alpha |
| c11 | (0.0625, 1, 1, 1): bump uv scale |
| c12 | camera position |
| c13-c15 | world matrix rows |
| c20-c22 | world 3x3 (transposed) for the basis |

## Vertex shader (waterbumpfresnal)
```
p     = (v0 * c4) * (1-w) + (v4 * c4) * w          // blended local position
oT0.xy = 0.0625 * p.xz                             // bump uv: tiles every 16 m
P     = p + c5 (patch offset), world = P * c13..c15
E     = c12 - world                                // to the eye, put in oT1.w / oT2.w / oT3.w
basis = (v1,v2,v3) * (1-w) + (v5,v6,v7) * w        // rows, then * c20..c22 -> oT1.xyz, oT2.xyz, oT3.xyz
oD0.rgb = c9.rgb
oD0.a   = c9.w + (1 - c9.w) * (1 - dot(normalize(E), v8))    // Fresnel: 0.6 looking down, 1.0 grazing
```

## Pixel shader (D3DPIXELSHADERDEF built at 0xa1ee0)
- PSTextureModes 0x64621: t0 PROJECT2D (bump map), t1 DOTPRODUCT, t2 DOTPRODUCT, t3 DOT_RFLCT_SPEC
  (= texm3x3vspec: N = basis * (t0.rgb * 2 - 1), reflect E about N, cube lookup in t3). PSDotMapping 0x111
  (signed -1..1).
- combiner 0: R0.rgb = t3, R0.a = t0.a.
- final: rgb = R0.rgb * v0.rgb (reflection x water colour), alpha = v0.a (Fresnel).

## water.dat and patches (0xcde12, 0xcdc20, 0xcdb60)
- header 0x1c bytes: u32 box count, u32, **f32 wave height scale** (copied to c4.y; bracken 1, silver 4,
  fuusoku 2.5, mars 0 = flat), then 40-byte boxes: u32 a, u32 b, 0, 0, f32 min[3], f32 max[3].
- boxes with `a & 1` are physics only: no water surface is drawn for them (the visible water there is the
  track's own meshes, e.g. bracken's "FLATwater"). Only fuusoku / alt_fuusoku have boxes with a = 0.
- for the other boxes: sample x and z every 8 m from min to max, snap to the 128 m grid
  (floor(v / 128) * 128), skip duplicates (same x and z), max 256 patches: (b, x, max.y, z). b = cube map
  slot of 0x50f880 (loaded by 0xcdcf0): 0 envdata\default.dds, 1 waterref1, 2 waterref2, 3..7 custom1..5; the draw
  binds it at 0xce5dd (the files differ on every track). Argon: ModelBuilder::cube, water.frag's tex[1..3].

## Wave frames (0xcd4f0, 30 frames, 17 x 17 vertices)
- frame k, t = 12 k degrees: `h(i, j) = 5000 sin(45 i + t) + 5000 cos(45 j + t) + 2000 sin(t - 90 i) + 2000 cos(t - 90 j)`,
  rounded to a short; position shorts (i, h, j) -> (8 i, h / 32767 * scale, 8 j). The waves repeat every
  16 cells, so patches tile.
- normals: d1 = h[j][i+1] - h[j][i-1], d2 = h[j+1][i] - h[j-1][i] (index -1 wraps to 16, +1 to 0),
  a = normalize(16, d1 * scale / 32768, 0), b = normalize(0, d2 * scale / 32768, 16), n = -normalize(a x b).
  These normals are stream 2 (v8): only the Fresnel term reads them.
- The heights are x87 sin / cos stored with fistp (round to nearest, ties to even). The index buffer (0xcdac5): per
  quad a = 17 j + i, triangles (a, a + 1, a + 17) and (a + 17, a + 1, a + 18).
- basis per vertex (0xcd170 with 0xccf40, the stride-0x2c vertex's +8 / +0x14 / +0x20 = v1 / v2 / v3): each triangle
  adds, axis by axis, the same shares as the road effect's 0xa2bb0 ([models/effects.md](../models/effects.md) "Effects") over position
  (8 i, h x scale / 32768, 8 j) and uv = position / 16: for axis c, (A, B, C) = e1 x e2 of the edges (dc, du, dv); if
  |A| > 1e-12, tangent.c += -B / A, binormal.c += -C / A. Then, in one pass k = 0..16, the column ends (k, 0) /
  (k, 16) and the row ends (0, k) / (16, k) both take their sum (the tile repeats; the corners collect in that order),
  tangent and binormal are normalised and v3 = tangent x binormal, not flipped: on flat water (1, 0, 0) x (0, 0, 1)
  = (0, -1, 0), pointing down. The reflection does not depend on the sign, but the bump's sideways tilt does.
- The vertex program blends both frames' rows (v1..v3 x (1 - w) + v5..v7 x w) and puts them through the world's
  rows c20..c22: oT1 = (T.x, B.x, N.x) etc. for the identity world, so the pixel's normal = T n.x + B n.y + N n.z.

## Physics (0xce6b0, called by the craft code 0x50ece..0x51342, hover 0x5f29b, water 0x613d4, 0xae6b5, the AI 0xe3a66)
`bool water_depth(x, y, z, seconds, float *depth)`, seconds = the game clock 0x50e430 + 0x50e434 (reset at the race
setup):
- game frame f = round(seconds x 60) (fistp); wave frames a = (f >> 3) % 30, b = a + 1 (0 after 29),
  w = (f & 7) x 0.125.
- the first box with min.x <= x < max.x and min.z <= z < max.z and y >= min.y (no upper bound) answers:
  depth = y - (max.y + wave), wave = 0 for a box with flags bit 0, else: u = (x - floor(x / 128) x 128) / 8,
  v likewise from z, cell (i, j) = (floor u, floor v) % 16, fractions clamped to 0..1; each corner's height = frame a's
  short x (1 - w) + frame b's x w; bilinear over the cell (x fraction first); x 1 / 32768 x scale, scale = water.dat's
  header float (0xcdefd copies it to 0x1a84f8 before building the frames; the render uses 1 / 32767).
- Only fuusoku / alt_fuusoku have boxes without bit 0, so only there do the waves change the physics.

## Port
Argon: src\app\app_track.cpp (patches, frames, textures; the heights from logic\water_waves.cpp, the basis from
render\water_basis.cpp, the Fresnel normal in the w slots of the frame buffer), shaders\water.vert / water.frag. The physics:
SimWorld::waterDepth = logic\water_waves.cpp waterDepthAt, its clock counted per 60 Hz step from the race setup
(resetDriver), as the game's from 0x3d34f / 0x3e870 (the exact offset between the two is not checked).
Race reports (`--menu --quick-race N --race-go --ai-race --drive-time 60`) before / after: Silver, Kalau, Flames,
Bracken, Draco unchanged (their boxes are flat; the new bounds made no difference); Fuusoku changed (Tara 349 -> 406
km/h at 60 s, the order 2 1 3 4 5 0 -> 1 2 5 4 3 0), and with the wave scale forced to 0 it is again identical to the
old report, so the waves alone make the difference. Fuusoku and alt_fuusoku over 200 s: 3 laps, no respawns.

## Render states (water_draw 0xce1c0)
Set by the draw itself (each setter skips the write when its cache already holds the value, unless 0x50e2f0 forces it):
- cull: 0xce275..0xce2a7: cache 0x50e398 = 0, SetRenderState_CullMode(D3DCULL_NONE) (0x10a3a0): both sides drawn.
- fog: 0xce308 = 0xa8450 in mode 0 (eax = 0, the three arguments 0): D3DRS_FOGENABLE shadow 0x118890 = 0 (dirty bit
  0x2000, cache 0x50e384). The vertex shader writes no oFog either: the water is never fogged.
- blend: 0xce312 = 0xa9530(1) (cache 0x50e32c): NV2A 0x304 BLEND_ENABLE 1, 0x344 SFACTOR SRC_ALPHA (0x302), 0x348
  DFACTOR ONE_MINUS_SRC_ALPHA (0x303), 0x350 EQUATION FUNC_ADD (0x8006) (shadows 0x11880c / 0x118818 / 0x11881c /
  0x118848). The back buffer is R5G6B5 ([models/README.md](../models/README.md)), so only the colour matters.
- 0xce31c = 0xa2390(1): lighting off (0x50e324, D3DRS_LIGHTING 0x1188b8 = 0; no effect with a vertex shader),
  stage 1 texture NULL (0x10cf90(1, 0)), cube slot 1 on stages 2 / 3 (each patch rebinds its own, 0xce5dd),
  stages 0 / 1 address wrap (0x118520 / 0x118524 = 1), stages 2 / 3 clamp (0x118620 / 0x118624, 0x1186a0 / 0x1186a4
  = 3), SetPixelShader(0x2907ec) (0x10b8d0).
Inherited (not set by the draw):
- callers: the race view 0x93490 (call 0x93849) and 0x98db1 (the same order). Both: depth / stencil cleared after
  the sky (0x93777: Clear flags 3), the track fog (0x937e3), the crafts, the render list's opaque buckets 0..2
  (0xbbc40: track and models through cmm_draw_mesh 0xb6e60), the water, then with one player (0x52cd8c = 1) the ship
  shadows (0x9c380 per craft, 0x938a0), then the translucent bucket 4 (0xbbf60: the track's see-through meshes).
- cmm_draw_mesh ends every drawn mesh with 0xbae10 (0xb7902, while 0x50e327 is set: always outside the shadow
  render): D3DRS_ZWRITEENABLE 1 (0xa8640(1)), ZENABLE = D3DZB_USEW (0xa86b0(1) -> 0x10b3e0(2)), NV2A 0x340 ALPHA_REF
  0. So the water is depth tested and **writes depth**.
- depth func: LESSEQUAL (NV2A 0x354 = 0x203), written once at device set-up (0xa69b3, the only write).
- alpha test: whatever the last opaque mesh left (0xb6fb9: off in pass 0, or on with surface flag 0x2000000), with
  ALPHA_FUNC GREATER (0x33c = 0x204, set once at 0xa69e0) and ref 0 (0xbae10). The water's alpha is at least 0.6, so
  it never rejects a pixel.
- depth bias: none. SetRenderState_ZBias (0x10a5e0: D3DRS 0x4d..0x51 = NV2A 0x384 / 0x388 / 0x330..0x338) is called
  only by 0xa9b30 (the ship shadow 0x9c380: 1 around its draw, then 0) and 0xb1930 (0), so it is 0 here.
- no mode switch: the same states above and under water.
Restored afterwards: only streams 1..3 cleared (0x109a30(i, 0, 0)) and SetPixelShader(0) (0xce67c). Blend, fog and
cull stay as set; the next setters see them in their caches (the shadows set their own, 0x9c4c0; each translucent
mesh sets blend, cull and depth in 0xba4f0 and its fog in its surface setup).
Argon (src\app\app_gpu.cpp createLineAndWaterPipelines, render\scene_recorder.cpp drawWater): alpha blend, depth test
nearer-or-equal (argon's reversed depth), depth write on, no culling, no fog (water.frag), and the ship shadows
drawn after the water as the game does (before: shadows first, a strictly nearer depth test and the track fog).
