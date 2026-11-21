# Ghidra Jython: water rendering names. See docs\water\README.md.
from ghidra.program.model.symbol import SourceType
fm = currentProgram.getFunctionManager()
names = {
    0x000ce1c0: ('water_draw', 'Draws the water: bump frame (f/2)%60 on stage 0, waterbumpfresnal vertex shader, pixel shader 0x2907ec,\n'
                 'blends wave frames (f/8)%30 and +1 with w = (f&7)*0.125 (c8), constants c4 scale (262136,3,262136),\n'
                 'c9 colour (0.5,0.7,1.0,0.6), c11 bump uv 0.0625, c12 eye; per visible patch: cube maps on stages 2/3,\n'
                 'c5 = patch pos - 4, DrawIndexedVertices(TRIANGLELIST, 0x600). See docs\\water\\README.md.'),
    0x000cd940: ('water_load_shaders', 'Loads data\\testdata\\waterbump000..059.dds, creates waterbump.xvu (0x42b4bc, unused) and\n'
                 'waterbumpfresnal.xvu (0x42c4d0) with declaration 0x1a8528, allocates the water index buffer.'),
    0x000a1ee0: ('water_create_pixel_shader', 'D3DPIXELSHADERDEF: modes 0x64621 (2D, dot, dot, DOT_RFLCT_SPEC = texm3x3vspec),\n'
                 'dot mapping 0x111, R0 = t3 (rgb) / t0.a, final = R0.rgb * v0.rgb, alpha v0.a. Handle -> 0x2907ec.'),
    0x000a2390: ('water_setup_stages', 'Cube maps (table 0x50f880) on stages 2/3, address modes, SetPixelShader(0x2907ec).'),
    0x000cd170: ('water_build_wave_frame', 'Builds one precomputed wave frame (heights / basis / normals). Not decoded yet.'),
}
labels = {
    0x0042c4d0: 'g_water_vs_bumpfresnel',
    0x0042b4bc: 'g_water_vs_bump',
    0x002907ec: 'g_water_ps',
    0x00447b68: 'g_water_bump_textures',
    0x0042b4c8: 'g_water_basis_frames',
    0x0042b4c0: 'g_water_patch_count',
    0x0042b4d0: 'g_water_patches',
}
tx = currentProgram.startTransaction('QR water labels')
done = 0
try:
    for a, (n, cmt) in names.items():
        f = fm.getFunctionAt(toAddr(a))
        if f is None:
            print('no function at %x' % a); continue
        f.setName(n, SourceType.USER_DEFINED)
        old = f.getComment() or ''
        if cmt not in old:
            f.setComment((old + '\n' if old else '') + cmt)
        done += 1
    st = currentProgram.getSymbolTable()
    for a, n in labels.items():
        st.createLabel(toAddr(a), n, SourceType.USER_DEFINED)
        done += 1
    print('QR water labels applied: %d' % done)
finally:
    currentProgram.endTransaction(tx, True)
