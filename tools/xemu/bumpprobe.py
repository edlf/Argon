"""Probe the road bump effect's constants in a race (xemu GDB stub on tcp 1234, tools\\xemu\\run_xemu.ps1).

usage: python -I bumpprobe.py [timeout_s=10]
Breaks at the "Diffuse Specular Bump Map" draw setup (0x7d6e0) to see whether the race uses it, then at its
SetVertexShaderConstant calls: c7 (0x7d862, the light) and c10 (0x7d9f6, the eye); prints the view matrix 0x50e1f0
and the pixel shader's colour inputs (ambient 0x50e2f4, sun 0x50e2f8) for comparison with the viewer.
"""
import os, struct, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from qrdbg import Gdb

SETUP, LIGHT_CONST, EYE_CONST = 0x7d6e0, 0x7d862, 0x7d9f6


def floats(g, addr, n):
    return [round(v, 4) for v in struct.unpack('<%df' % n, g.read(addr, 4 * n))]


def main():
    timeout = float(sys.argv[1]) if len(sys.argv) > 1 else 10.0
    try:
        g = Gdb()
    except OSError as e:
        raise SystemExit('no xemu GDB stub on 127.0.0.1:1234 (%s): start xemu with tools\\xemu\\run_xemu.ps1' % e)
    try:
        stop, _ = g.bp(SETUP, timeout)
        if not stop:
            print('0x7d6e0 not reached in %.0f s: no bump-mapped mesh was drawn (in a race? Fuusoku / Solar have none)'
                  % timeout)
            return
        print('0x7d6e0 reached: the bump effect is drawn')
        _, regs = g.bp(LIGHT_CONST, timeout)
        print('c7 (light)', floats(g, regs['edx'], 4))
        _, regs = g.bp(EYE_CONST, timeout)
        print('c10 (eye) ', floats(g, regs['edx'], 4))
        print('view 0x50e1f0', floats(g, 0x50e1f0, 16))
        ambient, sun = struct.unpack('<II', g.read(0x50e2f4, 8))
        print('ambient %08x, sun colour %08x' % (ambient, sun))
    finally:
        g.cont()


if __name__ == '__main__':
    main()
