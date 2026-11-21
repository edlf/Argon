# Ghidra Jython: names/comments for the mesh render-state code (blend, alpha test, depth, cull).
from ghidra.program.model.symbol import SourceType
fm = currentProgram.getFunctionManager()
names = {
    0x000b6e60: ('cmm_draw_mesh', 'Draws one CmmMesh. Surface block (mesh+0x20, 48 bytes):\n'
                 '+0x08 blend descriptor: bit0 blend on, bits0+1 = translucent pass (param_3 != 0, alpha test on);\n'
                 '+0x0c flags: 0x2000000 alpha test ref 0x80, 0x400000 shadow hull, 0x20000 sorted last (key 0), 0x40000 / 0x80000 / 0x100000 special paths.'),
    0x000ba4f0: ('mesh_apply_surface_state', 'surface +0x08 & 1 -> neon_setup_render_state(desc) else blend off;\n'
                 '+0x0c: 0x10 two-sided (cull none), 0x10000 z-test off, 0x8 z-write off, 0x2000 fog variants,\n'
                 '+0x04 == 1/2 extra setup (FUN_000ba370 / FUN_000ba1d0).'),
    0x000ba660: ('mesh_apply_material_stages', 'CmmMaterial (0xec): 4 stages of 0x34 from +0x0c. stage flags: 1 uv scroll\n'
                 '(+0x28 speed, +0x30 offset), 8 env/texgen (TexCoordIndex | 0x30000), 0x40 clamp U, 0x80 clamp V, 0x100 ?'),
    0x000a9530: (None, 'Blend descriptor -> NV2A blend state. 0 off, 1 SA/1-SA, 2 ONE/ONE, 3 ONE/ONE sub, 4 ONE/ONE revsub;\n'
                 'else bits 2..13 src factor (4 ONE,8 SC,0x10 1-SC,0x20 SA,0x40 1-SA,0x80 DA,0x100 1-DA,0x200 DC,0x400 1-DC),\n'
                 'bits 16..27 dst factor (same codes << 14), bits 28..30 equation (0 add,1 sub,2 revsub,3 min,4 max).'),
    0x000a94e0: ('rs_alpha_test_enable', 'NV2A 0x300 ALPHA_TEST_ENABLE'),
    0x000a8640: ('rs_depth_write', 'NV2A 0x35c DEPTH_MASK'),
    0x000a86b0: ('rs_depth_enable', 'ZEnable 2 / 0'),
    0x000a8ba0: ('rs_cull_default', 'CullMode CCW (0x901), or CW when mirrored'),
    0x000a8c60: ('rs_cull_none', 'CullMode NONE'),
    0x000a8810: ('tss_address_u', 'deferred texture state ADDRESSU: 1 wrap / 3 clamp'),
    0x000a88a0: ('tss_address_v', 'deferred texture state ADDRESSV: 1 wrap / 3 clamp'),
    0x000cd4f0: (None, 'water.dat: u32 count, 0, 1.0f, 4x0 (0x1c hdr); count x 40: u32,u32,0,0,min[3],max[3].\n'
                 'Surface = top face; drawn procedurally (17-wide grid, waterref1/2 cube maps).'),
}
tx = currentProgram.startTransaction('QR render labels')
try:
    for a, (n, cmt) in names.items():
        f = fm.getFunctionAt(toAddr(a))
        if f is None:
            print('no function at %x' % a); continue
        if n and not f.getName().startswith('neon_'):
            f.setName(n, SourceType.USER_DEFINED)
        old = f.getComment() or ''
        if cmt not in old:
            f.setComment((old + '\n' if old else '') + cmt)
    print('QR render labels applied')
finally:
    currentProgram.endTransaction(tx, True)
