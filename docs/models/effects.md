# Models: effect records

## Effects (records of 0x5bc bytes)
A material selects one with +0xe0 (-1 = fixed function). Every field the game reads (cmm_fixup_pointers 0xb6840..0xb68ae,
0xb6ca0, 0xaa810, create_vertex_shader 0xaaa10; `tools\cmm\record_survey.py CmmEffect`: nothing else is non-zero):

| off | meaning |
|---|---|
| 0x000 | name (0x40 bytes), looked up in the effect registry by FUN_000aa5f0 |
| 0x044 | flags: bit 0 use it (else the mesh's vertex shader is 0 and it is never drawn, 0xb6d5f); 1 in all 165 records |
| 0x050 | size of its vertex shader program (NV2A .xvu, stored after the records in order; `tools\cmm\xvu.py`) |
| 0x054 | the program, rebuilt at load (0xb6859) |
| 0x19c | run time: the vertex declaration the prepare passes to 0xaa810 (up to 0x100 dwords) |

The registry (0x292d5c + 0x418, count +0x414, 0x5c-byte entries made by 0x7db10 through 0xaa010 / 0xaa1a0 /
0xaa310 / 0xaa480): name (0x40), +0x40 prepare, +0x44 draw, +0x48 a call after the DrawIndexedVertices (0xaa6b0,
0xb7489: the bump effects' 0x7ca10), +0x4c release (0xb5f36, at model release: the hierarchy effects' 0x7a3b0 frees
their copied buffers), +0x50 stream 0 stride, +0x54 the vertex shader handle, +0x58 its use count. The handle is created once, from the first model's program and
declaration (0xaaa10); every later model with that name shares it. Per record the model keeps a 0x48-byte slot at
model +0x3a4 + 0x48 i: +0x00 the registry index, then a parameter block (+0x04 flags, +0x08..+0x24 the four stages'
scrolled uv offsets, +0x40 the clock, +0x44 a colour) that mesh_apply_material_stages (0xba6b6, 0xbaa51..) fills only
for the flag bits set (1/2/4/8 the stages' offsets, 0x100 the clock, 0x200 the bone's tinted colour, 0xb73dc).
No code in the XBE writes those flags (a scan of every [reg + 0x3a8] access), so the block stays as the model was
allocated. The prepare runs once per mesh at load (0xb6ca0) with (header, mesh, the bone record's +0x10c); the
draw (0xaa660, from cmm_draw_mesh 0xb7425) sets stream 0 and the shader, then calls it with (header, mesh, x): x is
the parameter block for a morph mesh (mesh flag 2) and otherwise the bone record's tinted colour +0x10c.

Which meshes use which effect (`python -I tools\cmm\effect_survey.py`): Diffuse Specular Bump Map 548 meshes (13
track models), Color Scale No Texture 269 (56 files), SpecAlphaHierarchy - 1 Tex 254 (53), Specular Bump RGBA 50
(19), Hud Lens 19 (5), Keyframe P - 2 Tex 18 (explodefirelarge only), Specular Bump 14 (12), Spheremap Decal 5 (5),
Fast Hierarchy - 1 Texture 1 (Yuri's shadow hull). Registered but in no file: Trail, Water Trail (the code's own
trails), SpecBump Hierarchy - 1 Tex, Keyframe P - No Tex, Keyframe PN - No Tex, Keyframe PN - 1 Tex.

- **Specular Bump** (0x7ba90 / 0x7bb80): vertex = pos, normal, uv, tangent basis (0x44); stage 0 = normal map
  with the material's stage-0 scroll (c15), basis x world (c7..c9), eye (c4); pixel shader 0x2907f0
  (texm3x3vspec): rgb = cube reflection, alpha = normal-map alpha. Cube map = mesh +0xa8, a slot of the environment cube table (below).
  Bracken "FLATwater" (its tex 23 is an animated normal map, texture type 8).
- **Specular Bump RGBA** (0x7bd50 / 0x7be50): the same plus a vertex colour; pixel shader 0x2907ec: rgb =
  reflection x vertex colour, alpha = vertex alpha. Bracken "popwater".
- **Hud Lens** (0x7a600 / 0x7a6a0): the mega-up's glass star (objects\megaup obj21, no stages). Vertex
  program: oT0 = the screen position of (pos + normal x c7.x), c7 = (-0.125, 0, 0, 0), over the vertex's own w (pixels
  into a linear texture, + c6), oD0 = vertex colour x c4 (255: white wherever not black). No pixel shader; stage 0 =
  the echo texture 0x50e160 and blending off (0x7a803: NV2A 0x304 SET_BLEND_ENABLE = 0, whatever the surface's
  descriptor says). The race scene copies the back buffer into the echo after the 3D view (0x93aa8) and only then
  flushes the sorted see-through bucket (0x93c49) the star is in (surface flag 0x800000), so it shows this frame
  behind it, pulled inwards, like glass. Argon: mesh_shading.glsl kind 6; SceneRecorder defers these meshes to the end of
  the world draw, FrameCopies::copySceneForLenses ends the pass, copies the image (FrameCopies' lens copy, bound as the scene set's
  binding 1) and resumes (Swapchain::resumePass). Read live (2026-11-16, `tools\xemu\combiners.py e4 b7370 b748e`):
  the program above with c4 = 255, c6 = (0.531, 0.531) (the half-pixel offset), c7 = (-0.125, 0, 0, 0); one combiner,
  r0 = t0 x v0, final combiner r0 (no fog), blending off; stage 0 the echo, linear R5G6B5 640 x 480 (format
  0x00011129, physical 0x00bde000). The copy (0x93aa8: GetSurfaceLevel 0x10e180, CopyRects 0x10c850 from [0x388f5c],
  the 640 x 480 R5G6B5 render surface at physical 0x00da0000) is 1:1. In xemu the echo's memory holds a smooth blur
  of the frame (2 x 2 pixel pairs), which makes the star a pale white glass; most likely xemu's surface download.
- **Specular Bump** cube (mesh +0xa8): a slot of the one global table 0x50f880 (0xa2180 / 0xa2390 bind it), loaded
  from the race track's envdata by 0xcdcf0: 0 default.dds, 1 waterref1, 2 waterref2, 3..7 custom1..5 (the front end
  loads data\defaultcubemap's, 0xa2040). Pickups and ships use it too (the mega-up's core reflects it; ModelBuilder::cubeOwner).
  The models use slots 0..2 (Draco has 2).
- **Diffuse Specular Bump Map** (0x7d5c0 / 0x7d6e0, after-draw 0x7ca10; shaders\diffusespecalpha.xpu): the road and
  ground meshes of every track but Fuusoku / Solar (vertex type 1, two textures: stage 0 colour, stage 1 normal map).
  - Prepare (0x7d5c0) copies each mesh into 0x50-byte vertices: pos, normal, uv set 0 (+0x18), tangent (+0x20),
    binormal (+0x2c), basis normal (+0x38), D3DCOLOR (+0x44), uv set 1 (+0x48). The basis (0xa2dd0 / 0xa2bb0): per
    triangle (strip degenerates skipped) and per axis c, the edges (dc, ds, dt) from corner 0 to 1 and 2 over **uv set
    1** give (A, B, C) = e1 x e2; if |A| > 1e-12 each corner gets tangent.c += -B / A, binormal.c += -C / A. Then per
    vertex: tangent and binormal normalised, basis normal = tangent x binormal (not normalised), negated when its dot
    with the vertex normal is < 0.
  - Vertex program (the record's .xvu): oT0 = uv 0, oT1 = uv 1, oD0 = vertex colour; with L = c7 (the sun vector
    negated and normalised, 0x2f250) and V = c10 - world position (c10 = the eye: inverse view x origin), both turned
    into object space by the world matrix's rows c12..c14: oT2 = (L.T, L.B, L.N), oT3 = H in the same basis with
    H = L + normalize(V).
  - Pixel shader: the handle 0x2907f8 that 0xa26c0 binds is loaded by 0xa2014 from **diffusespecalpha.xpu**
    (0x2907f4, bound by 0xa25a0 for another effect, gets diffusespecbump.xpu; checked by reading the shader back from
    xemu's memory with tools\xemu\bumpprobe.py's handle, 2026-11-15). `python -I tools\cmm\xpu.py <file.xpu>` prints the
    combiners: t0 colour, t1 normal map, t2 / t3
    project2d on the 64 x 64 **normalisation cube map** built at start-up (0xcb090, called from 0xa202c) and bound to
    stages 2 and 3 with clamp by 0xa269e (D3D turns the mode into a cube lookup for a cube texture). Constants
    (0x7d6e0): c0 = the scene ambient (0x50e2f4, alpha 0), c1 = light 0's colour (0x50e2f8), c2 = min(2 x c1, 1).
    `r1 = clamp(N.L) x v0 + c0`, `r0.rgb = r1 x t0` (the ambient is textured too), `r0.a = c0.a`,
    `r1 = clamp(N.H)^8 x t0.a` (squared three times, then masked by the colour texture's alpha), final
    `rgb = r0 x c1 + r1 x c2`, alpha r0.a, with N = 2 t1 - 1 (not renormalised). The material's combine codes,
    stage UV sets and scroll are not used. (diffusespecbump.xpu, without the t0.a mask and with the ambient added
    after the texture, belongs to the effect 0xa25a0 serves.)
  - In a race the scene ambient is the track's prop.dat ambient and light 0 its sun colour (0xb04a0; the render list
    stores them per entry, 0xbbecf); a mesh whose setup-1 surface has flag 4 makes its own colour x intensity x tint
    the ambient first (0xba370: some of Kalau's, Mount's and Mars's).
  - **No fog.** cmm_draw_mesh sets the surface's fog first (0xba4f0 at 0xb7216, the track's or the block's own), then
    calls the effect's draw through the record (0xaa660: `call [record + 0x45c]`, at 0xb7425) right before the
    DrawIndexedVertices (0xa7860 / 0xa7800), and 0x7d6e0 starts with the fog setter 0xa8450 in mode 0 (eax = 0 at
    0x7d73b: D3DRS_FOGENABLE shadow 0x118890 = 0, dirty bit 0x2000; modes 1 / 2 / 3 set it to 1 with the table mode
    0x118894 and the density 0x1188a0 or the range 0x118898 / 0x11889c). The vertex program writing no oFog is
    consistent with that, and the pixel shader's final combiner (rgb = r0 x c1 + ef, no fog input) would not apply
    fog either. The surface flags 0x40 / 0x80 (own fog / none) on these meshes therefore make no difference.
  - **Alpha.** The output alpha is c0.a = 0, and it is never seen: every Diffuse Specular Bump surface of the game's
    tracks (alt tracks included) has a blend descriptor of 0 or 0x100020 (bit 0 clear: opaque, alpha test off at
    0xb6fb2) and no surface flag 0x2000000, and the back buffer is R5G6B5 with D24S8 depth (neon_setup_presenter_params
    0xa6770: BackBufferFormat 5 at 0xa681a, AutoDepthStencilFormat 0x2a), so it has no alpha to keep: the frame copies
    (the echo 0x50e160 / 0x50e16c) and any DESTALPHA blend read 1. (No .cmm blend descriptor uses the DST_ALPHA codes
    0x80 / 0x100; in code only the translator 0xa9530 sets them, 0xa9897..0xa9a3e.)
  - Argon: mesh_shading.glsl kind 5 (app_track.cpp useDiffuseSpecularBump), the basis from src\render\bump_basis.cpp
    carried in the normal and uv sets 2 / 3 (free on vertex type 1), the cube map replaced by normalize(); no fog,
    alpha 1 (what a read of the game's frame gives). Checked live in xemu (tools\xemu\bumpprobe.py): c7, c10, the
    ambient and sun match argon's.
- **SpecAlphaHierarchy - 1 Tex** (prepare 0x7ca40, draw 0x7d0e0, after-draw 0x7ca10; registered at 0x7dbbb): every
  craft's and front-end ship's (data\characters\feship_*) hull meshes (vertex type 1, stage 0 colour on UV set 0,
  stage 1 normal map on UV set 1), one object and one track mesh; the pilots (data\characters\<pilot>) have no effect
  records. The Diffuse Specular Bump Map with a bone per vertex: the vertex program (the record's .xvu, 30
  instructions) picks the bone's world rows c[v7.x + 12..14] (`arl a0.x, v7.x`) and otherwise does what the one
  above does (oT0 uv 0, oT1 uv 1 = v8, oD0 colour, oT2 / oT3 the light c7 and half vector in the tangent basis v3..v5,
  c10 the eye). The draw sets the same constants (c4 = 1, c5 = 0, c6 the screen offset, c7 = unit(-sun 0x39110c),
  c10 the eye, pixel c0 = ambient 0x50e2f4, c1 = light 0 0x50e2f8, c2 = min(2 c1, 1)) and binds the same pixel shader
  (fx_dsbm_bind, diffusespecalpha.xpu) unless 0x1f0328 is set (only by the editor's shadow pass 0x9b820: no pixel
  shader, c0 = 20). The basis builder 0xa2a00 is 0xa2dd0 with its triangle shares (0xa27e0) taken over **UV set 0**
  instead of 1. On a craft the ambient / light 0 are the craft's own (0x59bc0, the render list sets them per entry).
  0xbb2a0 (craft load 0x506bf, scene models 0x96c75) only writes its last-but-one argument to +0x10c of those meshes'
  bones in the bone copy its last argument names (the scene models: 1 in copy 1, 2 in copy 2; use not traced). Argon:
  the same kind 5 (bump_basis.hpp bumpBasisUvSet, mesh_shading.glsl with the draw's own ambient / light 0).
- **Color Scale No Texture** (prepare 0x7a430, draw 0x7a4b0): the glow cones on every craft's and menu ship's pods
  (revol* bones, 33 vertices), Kalau's 72 lamp glows, Mars's, Mount's and the globe intro's cylinders; vertex type 0,
  no stages, blend 0x10007 (ONE / ONE), surface flags 0x7083f / 0x70851 / 0x70899 / 0x70841 / 0x70891 / 0x70033, all
  with 0x40000 (occlusion-tested). Vertex: position + D3DCOLOR (0x10 bytes). Program (6 instructions): oPos, oD0 =
  vertex colour x c4; nothing else. The draw: c4 = its third argument as RGBA / 255, which is the bone record's
  tinted colour +0x10c (0xb73f9), c6 the screen offset, and the fog off (0xa8450 mode 0). For a tested mesh
  cmm_draw_mesh writes +0x10c just before (0xb7109..0xb7187): each byte = round(tint byte (+0x104, 0xffffffff at
  load) x intensity), intensity = min(1, 2 x visible / N) as in "Flare occlusion". So the glow fades with how much of
  its point cloud is visible, unlit and unfogged (the fixed-function setup argon used before kept them at full
  brightness and fogged the lamps without surface flag 0x80). Argon: mesh_shading.glsl kind 7 (app_track.cpp usesColorScale), the tint x intensity
  per byte from flare_occlusion.hpp tintedChannel (SceneRecorder colorScaleTint); the alpha, also scaled in the game,
  never shows under ONE / ONE.
- **Spheremap Decal** (prepare 0x7a850, draw 0x7a920): Kutoo's and Jaya's front-end and character-select heads (mesh
  "kutoo3_kutoo_head" / "jaya3_jaya_headg", vertex type 1 / 2, two / three stages). Vertex 0x24: position, normal,
  D3DCOLOR, uv 0. Program (38 instructions): oT0 = uv 0 + c10 (0); the world position and normal from the world
  rows c70..c72; oD0 = min(min(colour x max(N.c5, 0) x c9 + c8, 1) + colour x max(N.c36, 0) x c37, 1) with c5 = the
  negated shell light direction (0x39110c), c8 / c9 the ints at 0x3910f4 / 0x391100 / 255 (1 with the editor's
  shadow flag 0x1f0328), c36 / c37 light 1 (0x388f64 negated, 0x388f70); oT1.xy = 0.5 - 0.5 x R.xy, R =
  unit(reflect(position - eye c4, N)) in world space (c11 = (1, 0, 0.5, -0.5)).
  - The shell light (0x3910f4.. ambient, 0x391100.. colour, 0x39110c direction; ints per channel): the start-up
    0x76d60 and 0xaf360 set ambient 0x40, colour 0xcc along unit(-0.5, -1, -1); 0xaf3d0 (a locked character on FE
    CHARSELECT) sets both black, same direction; nothing else writes them (the race draw 0x93490, the skinning setup
    0xa3df0, 0xb04a0, 0x59bc0 only read). So c8 = 0.251, c9 = 0.8, not D3D's ambient 0x707070. Light 1
    (0x388f64 / 0x388f70) is written only by fe_set_light1 0xa7ea0: a story scene's light ([sequences/README.md](../sequences/README.md) "The scene's
    light"), black along (0, 1, 0) for a locked character, zero (BSS) before any scene.
  - Stage 1's texture transform is set to COUNT2 (0x50e321) with whatever matrix stage 1 last had; it does not
    apply: the NV2A's texture transform is part of the fixed-function T&L, which a vertex program replaces, so
    COUNT2 only means no projective divide and stage 1 samples oT1 as it is.
  - Because the material has an effect, mesh_apply_material_stages skips the env-map setup, so stage 1 samples at
    oT1 whatever its flags (Kutoo's 0x108, Jaya's 0x100). Jaya's stage 2 (texture 6, ADD; Kutoo has two stages)
    reads oT2, which the program never writes: an unwritten output keeps the (0, 0, 0, 1) every output starts with
    (as xemu runs it; not checked on a console), so the stage adds the texel at (0, 0).
  - Argon: mesh_shading.glsl kind 8: stage 1 at 0.5 - 0.5 x the world reflection (the fixed path's u = (r.x + 1) / 2 is
    mirrored against it), stage 2 at (0, 0), and the program's lighting (spheremapDecalLighting, N.c5 per vertex
    from mesh.vert: vShellNdl), the shell light black when the front end's light 0 is (the locked character).
- **Fast Hierarchy - 1 Texture** (prepare 0x79c10, draw 0x7a050, release 0x7a3b0): only Yuri's LOW_YURI (the shadow
  hull, surface flag 0x400000). Program (20 instructions): a bone per vertex (v4.x selects c[12 + 3k], the draw loads
  three rows for every mesh of the model with the same effect, 0xaa900), oT0 = uv, oD0 = min((v3 x max(N.c7, c7.w) x c8 + c9) x c4,
  c8.w) with c7 = unit(-sun), c8 = light 0 (0x50e2f8), c9 = the ambient (0x50e2f4; 2 with 0x1f0328), c4 = 1, fog off.
  The ship shadow pass draws that hull only as a shape, so nothing to port.
- **Keyframe** effects: "Morph meshes" below.
