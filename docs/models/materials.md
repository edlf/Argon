# Models: materials and the texture table

## CmmMaterial (0xec bytes) — texture-stage combiners
`+0x00` stage count. Four stages of 0x34 bytes from +0x0c, applied by
`mesh_apply_material_stages` (0x000ba660) as Xbox D3D fixed-function texture
stages (pixel shader 0). The mesh's own stage count (+0x34) limits how many are used.
Material `+0xe0` (s32, usually -1): when >= 0 the env-map setup is skipped for every stage, so a stage
with flag 0x8 keeps its own UV set (bracken's road1 detail normal maps have 1 here).

| stage off | meaning |
|---|---|
| 0x00 | flags: 0x1 UV scroll, 0x8 env map (texcoords = camera-space reflection vector times the texture matrix 0x50e120 = inverse view rotation, translate (1,1,0), scale (0.5,-0.5,1): u = (r.x+1)/2, v = -(r.y+1)/2 with r the world-space reflection; stage_setup_envmap 0xbb110, envmap_update_texture_matrix 0xa9b60), 0x40 clamp U, 0x80 clamp V, 0x100 args swapped (`tss_args`: CURRENT op TEXTURE instead of TEXTURE op CURRENT) |
| 0x08 | texture index (same as the mesh's list) |
| 0x0c | UV set (TexCoordIndex) |
| 0x10 | colour combine code → COLOROP (`tss_color_op`) |
| 0x14 | alpha combine code → ALPHAOP (`tss_alpha_op`) |
| 0x04 | 0 in every file, not read |
| 0x08 | texture index: not read (the game uses the mesh's +0x38 list) |
| 0x0c | UV set (TexCoordIndex): set from here for stages 2 / 3 only (0xba831 / 0xba910); stages 0 / 1 keep 0 / 1 (0xbae10's reset) unless env-mapped (0xbb110: this | 0x30000). The stage's own number in every file |
| 0x18 | animation rate, frames/s: copied to the texture slot as it is before the mesh's textures are set (0xba716), so 0 keeps an animated texture on frame 0 (Alicia's craft, Mars and Silver have one such mesh each); the slot holds 60 from the load for meshes without a material. Argon: the same (it used to put 60 for 0) |
| 0x1c / 0x20 | UV scroll speed U / V (× 60 × time) |
| 0x24 / 0x28 | UV offset U / V |
| 0x2c / 0x30 | not read by anything (small values in 23 materials of Kalau, Mount and Kutoo's craft: `effect_survey.py`) |

Material words outside the stages: +0x00 stage count (not read: the mesh's +0x34 is), +0x04 0, +0x08 and +0xdc -1
in every file, +0xe4 / +0xe8 0; none is read. The material and stage reads are all in 0xba660, 0xbb110 and the
pushbuffer test of cmm_fixup_pointers (0xb6177..: a mesh whose stages' rate and scroll are all 0 can be pushbuffered).
With an effect the effect's draw replaces the stage setup's texture coordinates: the Specular Bump draws read the
stage-0 scroll and offset themselves (c15), Spheremap Decal's program makes stage 1's.

Combine codes: 0 MODULATE, 1 MOD2X, 2 MOD4X, 3 ADD, 4 ADDSIGNED, 5 ADDSIGNED2X,
6 SUBTRACT, 7 ADDSMOOTH, 8 BLENDDIFFUSEALPHA, 9 BLENDTEXTUREALPHA, 10 BLENDFACTORALPHA,
11 BLENDCURRENTALPHA, 12 PREMODULATE, 13 MODULATEALPHA_ADDCOLOR, 14 MODULATECOLOR_ADDALPHA,
15 MODULATEINVALPHA_ADDCOLOR, 16 MODULATEINVCOLOR_ADDALPHA, 17/18 MODULATE, 19 DOTPRODUCT3,
0x17 SELECTARG1, 999 DISABLE (alpha: 13..18 → MODULATE). Stage 0's CURRENT is the
diffuse colour (vertex colour for vtype 0..3, D3D lighting otherwise).
Typical uses: env-mapped stage `0x108` + MODULATEALPHA_ADDCOLOR = gloss reflection
masked by the base texture's alpha; BLENDCURRENTALPHA = terrain layer blending; the
"GO" screens = PREMODULATE + scrolling BLENDTEXTUREALPHA + BLENDCURRENTALPHA.

## Texture table (after the index data, header flag 0x1000)
16 bytes per texture: u32 type, u32 frame count, 2 more dwords (varied values, read by nothing). cmm_fixup_pointers
(0xb67a0) copies type and count (0 -> 1) into the model's 0x28-byte texture slot (+0x1c / +0x20; +0x24 rate = 60
until a stage sets its own, 0xba716); without header flag 0x1000 the slot gets type 0, one frame. The game compares
the type only with the code's own render-target types 0x60..0x63 (d3d_set_texture 0xcbb40, neon_create_texture
0xcb960), so the file's values are the tool's tags: 3 = normal map (A1R5G5B5; every one of the 76 is a bump
effect's normal map: Diffuse Specular Bump Map stage 1, SpecAlphaHierarchy stage 1, Specular Bump stage 0), 4 / 6 / 8
= animated (2..100 frames; 8 on animated normal maps such as Bracken's water). With frames > 1, `tex%d.dds` holds that
many complete DDS files back to back (each the file's size / frames, neon_create_texture 0xcb960); frame =
round(clock x stage rate) mod frames (`d3d_set_texture` 0x000cbb40), the clock = the whole seconds g_fe_clock_seconds
+ the float 0x50e434 or, for a mesh on a bone with its own clock (bone flag 0x80, 0xb7263 / 0xb7624), that clock.
The frame is rounded to nearest (argon's mesh_shading.glsl animationFrame, which used to truncate).

Bind cache (d3d_set_texture 0xcbb40..0xcbc3d, ESI = the slot, EAX = the stage): the frame is computed first
(fistp of clock x slot +0x24, then `div` by the count, so the low dword mod frames), then 0xcbbd2 compares the
slot pointer with the stage's entry of the table 0x50e374 (one dword per stage 0..3; stage 0x63 is read as 0) and
returns when they match, so the frame is thrown away; otherwise it stores the slot and calls SetTexture(stage,
frame's texture, or NULL unless the texture object's +0x84 = 4). Render-target slots (types 0x60..0x63) bind their
+0x0c with no check but also store the slot. A slot belongs to the model instance (+0x210 + index x 0x28, allocated
per instance at 0xb5adc), so only meshes of one instance with the same texture index on the same stage match.
cmm_draw_mesh (0xb6e60) calls mesh_apply_material_stages first, which writes each stage's rate into its slot
(0xba716), then binds stages below the mesh's +0x34 count (0xb7290 / 0xb72c1 / 0xb72f5 / 0xb7329, slots from the
+0x38 list) and empties the others (0xa9af0: entry = 0, SetTexture(stage, NULL); skipped when already 0 unless
0x50e2f0). Nothing else stores a slot. Entries are emptied: by the scene setup 0xa9be0 -> 0xa8750 (all four, also
the shadow pass 0xbe230), by the opaque render list before each of its meshes (0xbbc99, stage 0 only; the
translucent list 0xbbf60 empties nothing), once by the file-order draw 0xb80f0 before its two passes (stage 0), by
the occlusion test 0xa64f0 (stage 0) and by the 2D / HUD / particle / sprite draws (stage 0 or all). The
pushbuffered path (mesh flag 0x8000) calls SetTexture(0, NULL) without the table, but no file mesh has the flag.
So within the translucent list, and on stages 1..3 within the opaque one, a run of meshes binding one animated slot
on one stage all show the frame the first of them picked, with its rate (and its clock, the bone clock 0x439d94
when 0x439d90 is set).

Which meshes hit it (`effect_survey.py`'s list of meshes sharing an animated slot on one stage with different
rates, then a count in argon's race draw at 5 / 30 / 60 / 90 s on Mars, alt_mars, Draco, Solar, Kalau, Flames,
Mount and Silver): only Mars and its alt version in practice. Mars' 23 small
quads (meshes 160..167, 183..200: texture 14, 100 frames, stage 0, translucent) have rates 30..33 and 59..62, and
in the far-to-near list neighbours often follow each other (e.g. 184 at 59 drawn right after one at 30 shows 30's
frames); alt_mars has the same in meshes 23, 24, 29..34. The other candidates never came out adjacent in the runs:
the water / grid591 pairs at 30 and 60 on Draco (68, 69, 139, 193 vs 385), Solar (48, 49 vs 84), Kalau (63 vs 56),
Flames (44 vs 61), Mount (190) and Silver (454 at 0 among 30s). Alicia's gun quads (29..35, stage 1, texture
14, 2 frames: 34 has rate 0 among 60s, so drawn after 33 it would flicker with it) only draw with her weapon and
were not seen in the runs. Stage 1..3 sharing (Bracken, Draco,
Mount, Kalau) always has one rate, so it shows nothing.
Argon: render\texture_bind_cache.hpp holds the rule; SceneRecorder::drawMesh feeds it each drawn mesh's slots in
its draw order (a model's draw starts with no stage bound, a pass-0 mesh empties stage 0 first) and records the held
rates (`stageRates`, in the draw's record: render\mesh_draw_records.hpp) that mesh_shading.glsl's animationFrame
uses. It does not model the bone clocks (argon has no per-bone texture clock) or the file-order draw's carry from pass 0 into pass 1 (the data has no slot it would show
on).
Survey (`effect_survey.py`): type 0 x1907 (one frame), 3 x76, 4 x26, 6 x2 (60 frames), 8 x18 (60 frames). Argon uses frames and rate as the game does; type 3 only marks
its fixed-function bump stand-in, which no type-3 texture reaches (they all sit under a bump effect).
