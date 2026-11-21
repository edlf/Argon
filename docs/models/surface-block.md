# Models: the surface block (render state)

## Surface block (48 bytes, one per mesh with flag 0x20) — render state
Read by `mesh_apply_surface_state` (0x000ba4f0), `neon_setup_render_state`
(0x000a9530) and `cmm_draw_mesh` (0x000b6e60).

Draw order (track model, 0xb80f0): with no render list every mesh is drawn in file order, pass 0 then pass 1.
In a race the meshes go into a render list (0xbbb70) instead: blend descriptor bit 1 -> bucket 4 (translucent),
added by 0xbb820 sorted far to near on a key = the eye's distance (x and z, game camera +0x36c / +0x374) to the
farthest edge of the mesh's box (its vertex bounds, 0xb9270, model + 0x214, mesh +0x5c), the larger of the two,
at least 0.1; an entry goes before the first one whose key is <= its own (equal keys: reverse file order). A mesh
with surface flag 0x20000 goes in with key 0 instead (0xb02d1 / 0xb7efb), so it is drawn after every other one.
0xbbc40 draws buckets 0..2 (pass 0), 0xbbf60 bucket 4 (pass 1, alpha test on; alpha-0 texels still draw in xemu:
[megaup-cage.md](megaup-cage.md) "Blend descriptor").
In a race the track goes through 0xafcf0 instead: the meshes visible from the nearest sector are culled against
the view and listed (blend bit 1 -> see-through, pass 1); a see-through mesh with mesh flag 0x80 is keyed by the
eye's 3D distance to its sort point (mesh +0x4c, at least 0.1), the others by the far-edge key above. The sort
points are placed by hand: Silver's channel beds (meshes 460, 325) sit at y -27000, the speckle layer (462) at
-14000, the water (73) at -212, so the water is always drawn over its bed (13 of Silver's 471 meshes have 0x80).
Argon: app_frame.cpp sorts the track's pass 1 the same way.
Lighting. The device has one D3DMATERIAL8 for the whole game (0xa6ad7, the only SetMaterial: diffuse, ambient and
specular 1, emissive 0, power 4), D3DRS_SPECULARENABLE 0 and LOCALVIEWER 0 (0xa6997), COLORVERTEX 1 (0xa6a21), and
the material sources D3D's defaults (the default table 0x117628 applied by 0x112000; the game never writes
0x1188d8..0x1188e4): diffuse = vertex colour 1, specular = colour 2, ambient = emissive = the material. Light 0 is
directional (0xa6b32..0xa6b5b: diffuse 0.8, specular 0.5, ambient 0; in a race prop.dat's sun, 0xb04a0); light 1 only in the
front end. So a lit vertex is `clamp(D3DRS_AMBIENT + vertex colour x light 0 x N.L)`, alpha the vertex alpha (no
specular, the vertex colour does not touch the ambient). Per mesh (cmm_draw_mesh 0xb6e60 -> 0xba4f0, then the setups):
- 0xba4f0, any setup: flag 0x2000 with 0x100: D3DRS_AMBIENT = 0 (0xa8410) and light 0 black (0xa8140, direction
  (0, 0, 1)); 0x2000 without 0x100 or 0x200: light 0 black (0xa82d0). (Light 0 black also keeps the stages'
  colour ops: 0xbad5b's reset skips flag 0x2000.)
- Surface setup type 1 (0xba370): flag 4 with the bone record (0xba4f0's third argument, mesh flag 0x100: 0xb720f)
  makes colour +0x40 x intensity (+0x138, clamped to 255) x tint (+0x104, >> 8) the ambient (0xa8410), with 0x100
  light 0's colour (0xa82d0); the fog from the track (flags 0x80 off, 0x40 the block's own at +0x44); texture
  address wrap (0xa8930); lighting off (0xa8700's state 0x50e324 / 0x1188b8) unless flag 0x8000, which leaves it as
  it is: on, since cmm_draw_mesh turns it back on after every mesh (0xb790e: 0xbae10 also puts the scene ambient
  0x298da4, light 0, fog, depth and the stage states back). Every mesh of the game's files has a surface block.
- Setup 2 (0xba1d0): the same with the colour at +0x10, the fog at +0x14, lighting on (off with flag 0x20).
- The lens flare pieces (setup 1, 0x1bacb5 / 0x1ba8b5: 0x8000 lit, 0x2000 light 0 black, 4 the ambient = white x
  the sun's occlusion intensity) and the sun's FLARE_ (0x7acb5 / 0x7a8b5, the same lighting) have black vertex
  colours: the ambient is their whole colour, times their additive texture. Mount's Little_Flare lights (0x788b5) and
  the crafts' JFLARE / FLARE (setup 2, 0x72855) likewise.
- In a race the ambient and light 0 are prop.dat's, except on a craft: the ship draw (0x59bc0) sets, per channel from
  the craft's ground colour g ([physics/README.md](../physics/README.md) "Hover": +0x8ac), ambient = (g / 2 + ambient) / 2 and light 0 =
  min((g + sun) / 2, sun), the blue from the green sum (min((g.g + sun.g) / 2, sun.b), the game's slip). On a white
  road light 0 is the sun and the ambient about half way to grey 0x7f; on a dark one both drop. The render list
  stores those two per entry (0xbb700: 0x50e2f8 / 0x50e2f4) and sets them before the draw (0xbbecf).
The mega-up's cage gets its cycling pinks, purples and blues this way: its grey env texture ADDSIGNED2X that lit
colour as the model turns (the whole draw: "The mega-up's cage in the race" below).
Argon: render\d3d_lighting.cpp (surfaceLighting, craftLights; in the front end too, where an unlit surface keeps its
vertex colour: FE TITLE's letter faces), mesh_shading.glsl d3dLighting (material bits 32 lit, 64 / 128
ambient own / black, 256 / 512 light 0 own / black; the own colour in params.yzw x the bone tint), the craft's lights
in the draw's record (render\mesh_draw_records.hpp; SceneRecorder::craftLightsOf); the ground colour from TriangleSurface::colour.
