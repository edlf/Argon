# Ghidra Jython: names/comments for the texture-stage combiner setup and texture animation.
from ghidra.program.model.symbol import SourceType
fm = currentProgram.getFunctionManager()
names = {
    0x000a8cf0: ('tss_color_op', 'EDI = game combine code, ESI = stage -> D3DTSS_COLOROP via jump table 0x000a90a0:\n'
                 '1 MOD2X,2 MOD4X,3 ADD,4 ADDSIGNED,5 ADDSIGNED2X,6 SUB,7 ADDSMOOTH,8 BLENDDIFFUSEA,9 BLENDTEXA,\n'
                 '10 BLENDFACTORA,11 BLENDCURRENTA,12 PREMODULATE,13 MODA_ADDC,14 MODC_ADDA,15 MODINVA_ADDC,\n'
                 '16 MODINVC_ADDA,17/18 MODULATE,19 DOT3; 0x17 SELECTARG1; 999 DISABLE; other MODULATE'),
    0x000a9170: ('tss_alpha_op', 'Same codes -> D3DTSS_ALPHAOP (table 0x000a9488; 13..18 -> MODULATE)'),
    0x000a90f0: ('tss_args', 'stage flag 0x100: ARG1=CURRENT ARG2=TEXTURE, else ARG1=TEXTURE ARG2=CURRENT (colour+alpha)'),
    0x000cbb40: (None, 'Animated textures (texture table frames > 1): frame = (game time * slot+0x24 rate) % frames;\n'
                 'frames are complete DDS files concatenated in tex%d.dds'),
}
tx = currentProgram.startTransaction('QR stage labels')
try:
    for a, (n, cmt) in names.items():
        f = fm.getFunctionAt(toAddr(a))
        if f is None:
            print('no function at %x' % a); continue
        if n:
            f.setName(n, SourceType.USER_DEFINED)
        old = f.getComment() or ''
        if cmt not in old:
            f.setComment((old + '\n' if old else '') + cmt)
    print('QR stage labels applied')
finally:
    currentProgram.endTransaction(tx, True)
