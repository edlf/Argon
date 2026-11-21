# Ghidra Jython: env-map (reflection) texture stage setup. See docs\models\README.md (CmmMaterial), ported in shaders\mesh.vert and shaders\mesh_shading.glsl.
from ghidra.program.model.symbol import SourceType
fm = currentProgram.getFunctionManager()
names = {
    0x000a9b60: ('envmap_update_texture_matrix', 'Builds the env-map texture matrix 0x50e120 = transpose(view 0x50e1f0 with translation and w column zeroed)\n'
                 '* 0x50dce0: camera-space reflection vector -> world space -> uv. Result: u = (r.x + 1) * 0.5, v = -(r.y + 1) * 0.5\n'
                 '(r = world-space reflection), so the layer stays fixed when the camera only turns.'),
    0x000bb110: ('stage_setup_envmap', 'Stage flag 0x8: TEXTURETRANSFORMFLAGS = COUNT2, texture transform (stage 0..3) = 0x50e120,\n'
                 'TEXCOORDINDEX = uv set | 0x30000 (TCI_CAMERASPACEREFLECTIONVECTOR).'),
}
labels = {
    0x0050e120: 'g_envmap_texture_matrix',
    0x0050dce0: 'g_envmap_uv_matrix',      # translate(1,1,0) * scale(0.5,-0.5,1), built at 0xa6d43
    0x0050e1f0: 'g_view_matrix',
}
tx = currentProgram.startTransaction('QR env map labels')
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
    print('QR env map labels applied: %d' % done)
finally:
    currentProgram.endTransaction(tx, True)
