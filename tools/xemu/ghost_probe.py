"""Compare the Time Attack ghost crafts with the player's craft in a running game (xemu GDB stub on tcp 1234).

usage: python -I ghost_probe.py
Halts once and prints per craft (array 0x439d84, count 0x52cdbc): the ghost flags (+0x1144 ghost, +0x1129 hidden,
+0x188 no physics), then the model instance's bone records (craft +0x78c model, instance [craft +0x850] + 0x10,
records 0x1a0 at model +0x220[instance]): flags +0x102, tint +0x104, the tint x intensity +0x10c, intensity +0x138.
Last, the craft bytes that differ between the player (craft 0) and each ghost.
"""
import os, struct, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from qrdbg import Gdb

CRAFT_SIZE = 0x1150


def u32(g, a):
    return struct.unpack('<I', g.read(a, 4))[0]


def bone_records(g, craft):
    model = u32(g, craft + 0x78c)
    instance = u32(g, u32(g, craft + 0x850) + 0x10)
    count = u32(g, u32(g, model + 0x218) + 0x20)
    base = u32(g, model + 0x220 + 4 * instance)
    rows = []
    for b in range(min(count, 64)):
        r = base + 0x1a0 * b
        flags = struct.unpack('<H', g.read(r + 0x102, 2))[0]
        tint, lit = struct.unpack('<II', g.read(r + 0x104, 4) + g.read(r + 0x10c, 4))
        inten = struct.unpack('<f', g.read(r + 0x138, 4))[0]
        rows.append((b, flags, tint, lit, inten))
    return instance, rows


def main():
    g = Gdb()
    try:
        n = u32(g, 0x52cdbc)
        crafts = [u32(g, u32(g, 0x439d84) + 4 * i) for i in range(n)]
        dumps = []
        for i, c in enumerate(crafts):
            ghost, hidden = g.read(c + 0x1144, 1)[0], g.read(c + 0x1129, 1)[0]
            nophys = g.read(c + 0x188, 1)[0]
            print('craft %d @%08x ghost %d hidden %d nophysics %d' % (i, c, ghost, hidden, nophys))
            inst, rows = bone_records(g, c)
            print('  instance %d, %d bones' % (inst, len(rows)))
            for b, fl, tint, lit, inten in rows:
                print('    bone %2d flags %04x tint %08x lit %08x intensity %.3f' % (b, fl, tint, lit, inten))
            dumps.append(g.read(c, CRAFT_SIZE))
        for i in range(1, n):
            diffs = [o for o in range(CRAFT_SIZE) if dumps[0][o] != dumps[i][o]]
            print('craft %d vs 0: %d bytes differ; byte-sized ones (0/1):' % (i, len(diffs)))
            print('  ' + ' '.join('+%x=%d/%d' % (o, dumps[0][o], dumps[i][o]) for o in diffs
                                 if dumps[0][o] in (0, 1) and dumps[i][o] in (0, 1)))
    finally:
        g.cont()


if __name__ == '__main__':
    main()
