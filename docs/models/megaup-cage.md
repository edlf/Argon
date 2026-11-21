# Models: the mega-up's cage

## The mega-up's cage in the race (render list, fixed-function draw)
objects\megaup: obj22 the black core (opaque, blend 0x00100020, material +0xe0 = 1: an effect), obj21 the glass star
(Hud Lens, +0xe0 = 0), obj19 the cage frame (240 vertices, all 0xfe6e0000: red 110, alpha 254), obj20 a second copy of
the star's shape (228 vertices, the same positions as obj21, all grey 0xfeb2b2b2). obj19 / obj20: surface setup 2,
blend 0x00010007 (on, translucent, ONE / ONE add), flags 0x1b (drawn, collidable, no depth write, two-sided; no 0x20 so
lit; no 0x40 / 0x80 so the track's fog; no 4 / 0x2000), material +0xe0 = -1 (no effect), one stage: flags 8 (env), colour
code 5 ADDSIGNED2X (obj19) / 0 MODULATE (obj20), alpha code 0 MODULATE, rate 0; texture obj19 tex0 / obj20 tex1 (64 x 64
DXT1 greys: tex0 a dithered 0.16..0.45 averaging 0.33 with one white spot near uv (0.73, 0.27), tex1 dark with white
blobs). The way to the draw, in order:
1. The list build 0x93e20 (race scene): 0xb04a0 makes prop.dat's ambient the scene ambient (0x298da4, the cache
   0x50e2f4, D3DRS_AMBIENT 0x1188ec) and its sun light 0 (0x50e2f8, the direction normalised); then the track (0xafcf0),
   then the pickups (pickups_draw 0x8fbc0 at 0x93f1b / 0x93f3e: 0xb83e0 with instance index = pickup number + 1, identity
   rotation), and only then the crafts (0x93f94; their draw 0x59bc0 changes the ambient and light 0 and does not put
   them back). So a pickup's entries carry the track's lights.
2. 0xb7920 (0xb83e0 passes pass 1 for blend bit 1): blend bits 0 + 1 -> bucket 4 through renderlist_insert_sorted
   0xbb820 (far to near; obj19 and obj20 share a key, so obj20 goes first), the bone's world matrix (+0x40), entry
   +0x54 = 0x50e2f8 (light 0), +0x58 = 0x50e2f4 (ambient).
3. The translucent flush 0xbbf60 (from 0x938c8 / 0x93c5f, before the echo copy and the Hud Lens bucket 3): per entry
   ADDRESSU / V wrap on stages 0..3 (caches 0x50e318..0x50e31f), SetTransform(world), light 0's colour = entry +0x54
   (0xa82d0 at 0xbbecf), D3DRS_AMBIENT = entry +0x58, then cmm_draw_mesh(model, mesh, 1, instance); the stage-0 bind
   cache is not emptied.
4. cmm_draw_mesh 0xb6e60: mesh flag 0x20, blend bits 0 + 1, pass 1: alpha test on (0xa94e0(1)); the function stays
   GREATER (0x204, set once at 0xa69db) and the reference 0 (0xbae10's reset): alpha 254 passes. Mesh +0x40 is the FVF
   0x152 (XYZ | NORMAL | DIFFUSE | TEX1, 0xb6bd0 for vertex type 0), not -1, so this is the **fixed-function branch**
   (0xb704a -> 0xb74de..0xb7913), not the effect branch with its draw point 0xb7370 / 0xaa660: 0xba4f0 (0xb75fd),
   0xba660(model, mesh, no parameter block, the model clock +0x5e4) (0xb760d), the stage textures (d3d_set_texture /
   0xa9af0), the draw at **0xb78c8** (0xa7860 strip / 0xa7800 list), then 0xbae10 and lighting on (0xa8700(1)).
5. 0xba4f0: neon_setup_render_state(0x10007): blending on, SRCBLEND ONE, DESTBLEND ONE, BLENDOP ADD; cull none (0x10),
   z test on, z write off (8); setup 2 (0xba1d0): no flag 4, the track fog (0x298da8 / 0x298dac / 0x298db0 through
   neon_setup_fog), D3DRS_LIGHTING 1 (0x1188b8).
6. 0xba660: stage 0 args TEXTURE / CURRENT (0xa90f0, flag 0x100 clear: COLORARG1 = ALPHAARG1 = 2 TEXTURE, COLORARG2 =
   ALPHAARG2 = 1 CURRENT, which on stage 0 is the diffuse), COLOROP code 5 -> 9 ADDSIGNED2X / code 0 -> 4 MODULATE
   (0xa8cf0, jump table 0xa90a0), ALPHAOP 4 MODULATE (0xa9170), rate 0 into the slot (0xba716), wrap; stage 1 COLOROP
   999 -> DISABLE (0xba996). The env stage (0xbb110): TEXTURETRANSFORMFLAGS(0) = 2 (COUNT2), the TEXTURE0 matrix =
   0x50e120, TEXCOORDINDEX(0) = 0x30000 (camera-space reflection vector). Light 0 is the sun, not black, so the
   MODULATE reset of 0xbad5b does not run. SetVertexShader(the FVF) at 0xbadd1.
7. Set by nothing on the way (device init 0xa6880.., no later game write): SPECULARENABLE 0 (0xa6997, its only game
   write), LOCALVIEWER 0 (0xa69a3), COLORVERTEX 1 (0xa6a21), the material sources D3D's defaults (table 0x117628:
   0x1176fc specular COLOR2, 0x117700 diffuse COLOR1, 0x117704 ambient MATERIAL, 0x117708 emissive MATERIAL; the D3D
   library's colour-material word 0x10f240 keeps a COLOR1 source only while the vertex shader has a diffuse colour,
   flag 0x400, which the FVF has), the one material (0xa6ad7: ambient 1, emissive 0), light 0 directional with
   ambient 0 and specular 0.5 (unused), the texture filters (0xa690a.., one value for all stages), COLORSIGN 0; the
   texture factor is not an argument, stages 2 / 3 sit behind the disabled stage 1.

So, per vertex, diffuse = clamp(ambient + vertex colour x sun x N.L), alpha 254 / 255; per pixel obj19 adds
clamp(2 x (env + diffuse) - 1) and obj20 env x diffuse to the frame, fogged towards the track's fog colour first.
With Silver's lights (ambient (61, 58, 88), sun white) and tex0's 0.33 that is red 2 x (0.57 + 0.43 N.L) - 1, green
2 x 0.56 - 1 = 0.12, blue 2 x 0.67 - 1 = 0.35: magenta; Mars (ambient (88, 44, 41)) red. Argon draws exactly this
(mesh_shading.glsl kind 2: d3dLighting, then colourOp ADDSIGNED2X / MODULATE on the env uv; ONE / ONE, no depth write,
two-sided). In xemu the frames are paler, mostly white with pink / purple edges, so an input above differs at run
time; not settled. Live check (xemu GDB stub, a mega-up on screen): `python -I tools\xemu\ffprobe.py f0` (obj19; `e4`
obj20) breaks at 0xb78c8 for that mesh and prints D3DRS_AMBIENT (0x1188ec, expected the track ambient 0xffRRGGBB),
light 0 (D3DLIGHT8 at 0x292d48 + 0x60b4: diffuse = the sun, ambient 0; 0x50e2f8), LIGHTING / SPECULARENABLE /
LOCALVIEWER / COLORVERTEX and the material sources (0x1188b8..0x1188e4: 1 0 0 1, 2 1 0 0 2 1 0 0), the vertex
shader's flags ([[0x118510] + 0x380] + 4, 0x400 set), stage 0 (0x118520..: COLOROP 9 / 4, args 2 / 1, ALPHAOP 4,
TEXTURETRANSFORMFLAGS 2) and stage 1 (COLOROP 1), TEXCOORDINDEX (0x50e364: 0x30000), the matrix 0x50e120, blending
(0x11880c 1, 0x118818 / 0x11881c ONE / ONE) and the alpha test (0x118810 1, 0x118808 0x204, 0x118814 0). A value off
from these is the difference; if all match, the difference is in the GPU's lighting or texgen (compare a frame's
pixel against the formula). Left in argon, small at close range: the lighting per pixel (the game's per vertex),
the reflection from the eye position (LOCALVIEWER 0 may make the game's eye vector (0, 0, 1) in camera space) and
additive meshes fading with the fog (rgb x f) instead of taking the fog colour before the add.

Read live (2026-11-16, xemu, Silver, a mega-up in view): every value above is as expected, for both meshes.
D3DRS_AMBIENT 0xff3d3a58 (= the scene ambient 0x298da4), light 0 directional (0.577, -0.577, 0.577), diffuse white,
ambient 0; LIGHTING 1, SPECULARENABLE 0, LOCALVIEWER 0, COLORVERTEX 1, sources specular 2 / diffuse 1 / ambient 0 /
emissive 0 (front and back); the shader flags 0x400; stage 0 COLOROP 9 (obj19) / 4 (obj20), args TEXTURE / CURRENT,
ALPHAOP 4, transform COUNT2, TEXCOORDINDEX 0x30000; stage 1 COLOROP 1; ONE / ONE add, alpha test GREATER 0. The
texture matrix 0x50e120 is the camera rotation x (-0.5, -0.5) plus (0.5, -0.5) (it moves with the view). The one
D3DMATERIAL8 (0x292d48 + 0x6088..) holds diffuse / ambient / specular 1, emissive 0, power 4. The textures bound
(slot +8 = the texture object, its +0x94 -> the frames' D3D textures) are 64 x 64 DXT1 (format 0x06650c29) whose data
in memory is byte for byte tex0.dds / tex1.dds, and obj19's vertex buffer still holds 0xfe6e0000 on every vertex (no
run-time tint). So the inputs match argon's; tex0's bright spot is at (47, 19) of 64 (u 0.73, v 0.30), the
centre camera-facing faces map to is about 0.4. Argon now lights the fixed-function way, N.L per vertex
(mesh.vert vNdl), and reflects for its env stages with the non-local argon (the camera's forward axis, vReflFar),
but the cage stays magenta: D3D's documented maths on these inputs gives green 2 x (0.4 + 0.23) - 1 = 0.26.
The NV2A program the draw runs (`python -I tools\xemu\combiners.py f0`: the commands DrawIndexedVertices writes,
read from the pushbuffer at [[0x118510] + 0]): one combiner stage, r0.rgb = (t0 x 1 + 1 x v0 - 0.5) x 2
(ADDSIGNED2X), r0.a = t0.a x v0.a; the final combiner (last set before the draw) out = fog.a x r0 + (1 - fog.a) x
the fog colour, no specular term (the NV2A's specular enable is 1, but v1 is not combined); colour material 0x10
(diffuse from the vertex), scene ambient (0.239, 0.227, 0.345), emission 0, light 0 ambient 0. So the GPU runs the
documented maths too, and the fog: an additive mesh on the track's fog gets the fog colour added with distance
(argon used to fade it to black; surface fog modes below). At the distances the cage is seen on Silver (fog
density 0.0004) that is small, so its white in xemu is still not explained by the game's state.

| off | meaning |
|---|---|
| 0x00 | block size: 0x60 setup 1 (track models), 0x30 setup 2 |
| 0x04 | 1 / 2: extra per-mesh setup (0xba370 / 0xba1d0, "Lighting" above: with flag 0x4 and a bone record, the colour at +0x40 / +0x10 x instance intensity (+0x138) x instance tint (+0x104, /256) becomes the scene ambient (0xa8410, D3DRS_AMBIENT), so it shows on lit meshes; flag 0x100 loads it as light 0's colour instead (0xa82d0)) |
| 0x10 (setup 1: 0x40) | lighting block (0x20 bytes), see below |
| 0x08 | **blend descriptor** (below) |
| 0x0c | flags: 0x1 drawn (clear: 0xb6e60 skips the mesh in the race, e.g. the invisible walls and Solar mesh 175), 0x40000 occlusion-tested (flares, below), 0x80000 intensity from the parent bone (below), 0x2 collidable, 0x8 no depth write, 0x10 two-sided (no culling), 0x10000 no depth test, 0x20000 drawn last in its translucent bucket (sort key 0: 0xb02d1, 0xb7efb; Draco's sand over its grass), 0x2000000 alpha test (ref 0x80), 0x2000 light 0 black (with 0x100 the ambient too; 0x200 keeps the sun: 0xba5a7), 0x4 own colour as the ambient (0x100: as light 0), 0x8000 setup 1 lit, 0x20 setup 2 unlit, 0x40 own fog, 0x80 no fog, 0x400 placed relative to the camera, 0x800 faces the camera, 0x100000 billboard (slid towards the view axis, 0xbb270; these three: [effects/rendering.md](../effects/rendering.md) "Camera-facing meshes"), 0x800000 sorted translucent (render list bucket 3), 0x400000 shadow hull (below) |
| 0x14 / 0x18 | setup 1: surface type / reset (1: a craft whose hover ray or hull touches the mesh is put back on the track, [physics/README.md](../physics/README.md) "Reset request"); collision_build_from_cmm copies these two words of every block (in a setup 2 block they are the lighting's fog colour / density) |

Shadow hull (surface flag 0x400000): with 0x50e327 clear (0xa8750(0), set only by the ship shadow pass 0xbe230)
cmm_draw_mesh (0xb6ea8) draws only these meshes (all of them when 0x439f60, set by 0x9b936). Every craft has exactly
one: a low-poly copy of the whole ship (1000..2600 vertices) under its COCKPIT_ bone (LOW_TARA, LOW_YURI,
LOW_KUMIKO, objN; Vladimir's is a second bone named COCKPIT_). It lies on the real hull and, over the canopy glass, is
an opaque dome. In the race (xemu) the pilot shows through the glass, so the normal view does not draw it; what
hides it there is not found (0xb6e60 draws every mesh while 0x50e327 is set). Argon: Draw::shadowHull on craft
models, hidden in every draw but the ship shadow, which draws only it (ShipShadows::drawCaster). Alicia's front-end
character models also carry the flag on dozens of body meshes; they are drawn as usual.

Lighting block (setup 2 at +0x10, setup 1 at +0x40; read by 0xba1d0 / 0xba370 and cmm_draw_mesh):

| off | meaning |
|---|---|
| 0x00 | D3DCOLOR (power-ups: red 0xff0000, yellow 0xffd700, blue 0x0000ff, inner meshes 0xff6060 / 0xffd760 / 0x6060ff); flag 4: the scene ambient, with 0x100 light 0 |
| 0x04 / 0x08 / 0x1c | fog colour (D3DCOLOR), density, range: used instead of the track's fog with flag 0x40 (flag 0x80: no fog; 0xba1d0 at 0xba2ac / 0xba370 at 0xba44c pass mode, colour, density and range to 0xa8450). Argon: render\mesh_fog.hpp, GpuMaterial fog / fogShape, mesh_shading.glsl meshFog |
| 0x0c / 0x10 | radius (float) and point count of the occlusion test of flag 0x40000 (0xbb240 / 0xbb210) |
| 0x14 | billboard factor (float, 0xbb270): with surface flag 0x100000 the share of the mesh's offset across the view axis it keeps (1 in place, 0 on the axis, < 0 across it: the lens pieces -0.75 / -0.8 / -0.2 / 0.2, [effects/rendering.md](../effects/rendering.md) "Camera-facing meshes") |
| 0x18 | own fog mode (surface flag 0x40) for the fog setter 0xa8450: 0 off, 1 / 2 EXP, 3 LINEAR (2026 meshes 0, 9 meshes 3) |

Occlusion test (flag 0x40000, cmm_draw_mesh 0xb6e60): before the draw, the first N (+0x20, 32 on the ship
flares) of 256 fixed points uniform in a unit sphere (0xa63b0, made at start-up with rand) are drawn as 1-pixel
points through the mesh's world matrix with its axes scaled by the radius (+0x1c: 0.1 / 0.3 m), depth tested, no
depth write, inside a visibility test (0xa64f0). The previous test's result (0xa6750) sets the instance intensity
+0x138 = min(1, 2 x visible / N); at 0 the mesh is not drawn. So a flare fades when the hull or the track hides
it, and is gone off screen. +0x138 (and the test's query index +0x108) belong to the mesh's **bone record** (0x1a0
bytes; stored as 0 in every file; at load neon_model_load_skeleton 0xb9540, from cmm_fixup_pointers, sets
+0x138 = 1.0, the tint +0x104 = 0xffffffff, +0x108 = 999999 (no test yet) and +0x13c / +0x130 = 0; each instance
copies those records, 0xb96e0). Every mesh on a bone whose +0x138 is <= 0 is skipped (0xb71f8), not only the tested ones. With surface flag 0x80000 a mesh instead copies its parent
bone's +0x138 (0xb6e00: the mesh's bone, +0x180 parent, 0xb6e30: that bone's record) before the same "0 = not
drawn" check: the sun's lens-flare pieces c1..c5 (flags 0x1ba8b5 / 0x1bacb5) under the tested FLARE_ bone (Solar,
Fuusoku, Draco, Bracken, Flames and their alt versions), Darius's grid2..grid4 under grid1_*, explosion_type9.
They come first in the file, so they use the sun's result from the frame before. A bone not tested yet (+0x108 =
999999) reads 0 visible points, so a light is not drawn before its first result. Details: [effects/README.md](../effects/README.md) "Flare
occlusion". Argon: SceneRecorder::testedOcclusion, FlareQueries::readResults (Vulkan occlusion queries, read when the
frame slot comes round again; intensity per model, bone and ship).

Blend descriptor: bit0 = blending on. bit0+bit1 = drawn in the **second
(translucent) pass**, with alpha test ref 0; alpha-0 texels still draw in the game (seen in xemu: Solar's sun, whose
FLARE_ texture has a ring of alpha 0 with colour, shows no gap under ONE / ONE), so argon does not discard them. Values 1–4 are presets (1 = SRC_ALPHA /
1-SRC_ALPHA, 2 = ONE/ONE, 3 = ONE/ONE subtract, 4 = ONE/ONE reverse-subtract).
Otherwise bits 2..13 = source factor, bits 16..27 = destination factor, both coded
as 0x4 ONE, 0x8 SRC_COLOR, 0x10 1-SRC_COLOR, 0x20 SRC_ALPHA, 0x40 1-SRC_ALPHA,
0x80 DST_ALPHA, 0x100 1-DST_ALPHA, 0x200 DST_COLOR, 0x400 1-DST_COLOR (dst codes
shifted left by 14), bits 28..30 = equation (0 add, 1 sub, 2 rev-sub, 3 min, 4 max).
Examples: `0x00100023` alpha blend (translucent pass), `0x00010007` additive glow,
`0x00100020` blend codes present but bit0 clear → opaque.
