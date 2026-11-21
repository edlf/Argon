"""Log, in order, calls of tss_color_op (0xa8cf0: EAX = game combine code, ESI = stage, with the return address)
together with the mesh draw point 0xb7370 (EBX = mesh record: vertex count, flags), so each combine write can be
tied to the mesh drawn after it (xemu GDB stub on tcp 1234).
usage: python -I tsslog.py [hits=600] [first=draw|any]
first=draw (default) starts logging at the first hit of the draw point.
"""
import os, struct, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from qrdbg import Gdb

TSS, DRAW = 0xa8cf0, 0xb7370


def main():
    hits = int(sys.argv[1]) if len(sys.argv) > 1 else 600
    wait_draw = (sys.argv[2] if len(sys.argv) > 2 else 'draw') == 'draw'
    g = Gdb()
    for a in (TSS, DRAW):
        if g.cmd('Z0,%x,1' % a) != 'OK':
            raise SystemExit('breakpoint failed at %#x' % a)
    rows, last = [], None
    try:
        n = 0
        while n < hits:
            if last is not None:   # step off the breakpoint just hit
                g.cmd('z0,%x,1' % last)
                g.cmd('s', 5)
                g.cmd('Z0,%x,1' % last)
            g.cont()
            g._recv(10)
            r = g.regs()
            last = r['eip']
            if last == DRAW:
                nv, fl = struct.unpack('<II', g.read(r['ebx'] + 8, 8))
                wait_draw = False
                rows.append('DRAW mesh verts %d fl %#x' % (nv, fl))
            elif not wait_draw:
                ret = struct.unpack('<I', g.read(r['esp'], 4))[0]
                rows.append('     code %3d stage %d  from %#07x' % (r['eax'], r['esi'], ret))
            if not wait_draw:
                n += 1
    finally:
        for a in (TSS, DRAW):
            g.cmd('z0,%x,1' % a)
        g.cont()
    for row in rows:
        print(row)


if __name__ == '__main__':
    main()
