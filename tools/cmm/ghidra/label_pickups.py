# Ghidra Jython: pickup loader / draw names.
from ghidra.program.model.symbol import SourceType
fm = currentProgram.getFunctionManager()
names = {
    0x00078380: ('pickups_load', 'Parses d:\\config\\%spup.txt (PUPSTART/type/X/Y/Z/PUPEND) into 0x60-byte entries at\n'
                 '0x5293b0+0x1cf0 (pos), +0x1cfc type (0 blue,1 red,2 yellow,3 megaup,4 powerchip).\n'
                 'Y is raised per type: blue 3.84, red 3.08, yellow 3.93, megaup 5.97, powerchip 5.74 (0x1a3970..80).'),
    0x0008fbc0: ('pickups_draw', 'Draws pickups of one type: shared matrix 0x50a180, y += 0.5 + 0.5*sin(2*time + index), no scale'),
    0x0003e4a0: ('pickups_init', 'Loads objects\\powerup_*/powerchip/megaup models (_ee variants when DAT_001bbc16), then pickups_load'),
}
tx = currentProgram.startTransaction('QR pickup labels')
try:
    for a, (n, cmt) in names.items():
        f = fm.getFunctionAt(toAddr(a))
        if f is None:
            print('no function at %x' % a); continue
        f.setName(n, SourceType.USER_DEFINED)
        old = f.getComment() or ''
        if cmt not in old:
            f.setComment((old + '\n' if old else '') + cmt)
    print('QR pickup labels applied')
finally:
    currentProgram.endTransaction(tx, True)
