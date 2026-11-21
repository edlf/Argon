"""Diff the D3D state of two crafts' mesh draws in a running game (xemu GDB stub on tcp 1234).

usage: python -I craft_drawstate.py <craft a index> <craft b index> [lo=118000] [hi=119000]
For each craft (array 0x439d84): breaks at its draw (craft vtable draw 0x59bc0, ECX = the craft), prints the caller,
then breaks at the first mesh draw point inside it (cmm_draw_mesh 0xb7370, before the draw's return 0x59f74) and
reads the guest memory lo..hi (the D3D state caches). Prints the dwords that differ between the two crafts.
"""
import os, struct, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from qrdbg import Gdb

CRAFT_DRAW, MESH_DRAW, DRAW_RETURN = 0x59bc0, 0xb7370, 0x59f74


def set_bp(g, a):
    if g.cmd('Z0,%x,1' % a) != 'OK':
        raise IOError('breakpoint at %#x failed' % a)


def clear_bp(g, a):
    g.cmd('z0,%x,1' % a)


def run_to_stop(g, at):
    """Continue until a breakpoint hit; step off the breakpoint at 'at' first."""
    if at is not None:
        clear_bp(g, at)
        g.cmd('s', 5)
        set_bp(g, at)
    g.cont()
    g._recv(15)
    return g.regs()


def capture(g, craft, lo, hi):
    set_bp(g, CRAFT_DRAW)
    at = None
    for _ in range(200):
        r = run_to_stop(g, at)
        at = CRAFT_DRAW
        if r['ecx'] == craft:
            break
    else:
        raise SystemExit('craft %#x never drawn' % craft)
    caller = struct.unpack('<I', g.read(r['esp'], 4))[0]
    clear_bp(g, CRAFT_DRAW)
    set_bp(g, MESH_DRAW)
    set_bp(g, DRAW_RETURN)
    r = run_to_stop(g, None)
    clear_bp(g, MESH_DRAW)
    clear_bp(g, DRAW_RETURN)
    where = 'mesh draw' if r['eip'] == MESH_DRAW else 'draw returned (no immediate mesh draw)'
    print('craft %#x: called from %#x, stopped at %s' % (craft, caller, where))
    return g.read(lo, hi - lo)


def main():
    ia, ib = int(sys.argv[1]), int(sys.argv[2])
    lo = int(sys.argv[3], 16) if len(sys.argv) > 3 else 0x118000
    hi = int(sys.argv[4], 16) if len(sys.argv) > 4 else 0x119000
    g = Gdb()
    try:
        table = struct.unpack('<I', g.read(0x439d84, 4))[0]
        crafts = [struct.unpack('<I', g.read(table + 4 * i, 4))[0] for i in (ia, ib)]
        a = capture(g, crafts[0], lo, hi)
        b = capture(g, crafts[1], lo, hi)
        for o in range(0, hi - lo, 4):
            va, vb = struct.unpack_from('<I', a, o)[0], struct.unpack_from('<I', b, o)[0]
            if va != vb:
                print('%08x  %08x -> %08x' % (lo + o, va, vb))
    finally:
        for x in (CRAFT_DRAW, MESH_DRAW, DRAW_RETURN):
            clear_bp(g, x)
        g.cont()


if __name__ == '__main__':
    main()
