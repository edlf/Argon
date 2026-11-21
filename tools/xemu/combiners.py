"""Capture the NV2A commands one mesh's draw writes and decode its register combiners (xemu GDB stub, tcp 1234).

usage: python -I tools\\xemu\\combiners.py [verts=f0] [start=b78c8] [end=b78ef]
Breaks at cmm_draw_mesh's fixed-function draw point (0xb78c8, EBX = the mesh record) until a mesh with that vertex
count (+0x08, hex) is drawn, notes the D3D pushbuffer's write pointer ([[0x118510] + 0];0x10f120 reserves
from it), runs to just after the draw call (0xb78ef, after DrawIndexedVertices 0x108e30 and its lazy state flush
0x110700) and decodes every method written in between: the combiner stages' inputs / outputs, the final combiner,
the texture shader program, texgen and lighting. The mega-up's cage (objects\\megaup): obj19 = f0, obj20 = e4.
"""
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from qrdbg import Gdb  # noqa: E402

DEVICE = 0x118510
SOURCES = ('zero', 'c0', 'c1', 'fog', 'v0 diffuse', 'v1 specular', '?6', '?7', 't0', 't1', 't2', 't3', 'r0', 'r1',
           'spec+r0', 'EF')
MAPPINGS = ('', '1-', '2x-1 ', '-(2x-1) ', 'x-0.5 ', '-(x-0.5) ', 'signed ', '-')
SHIFTS = ('', ' -0.5', ' x2', ' (x-0.5)x2', ' x4', '?5', ' /2', '?7')
SHADER_MODES = ('none', '2D', '3D', 'cube', 'pass', 'clip', 'bump', 'bump lum', 'brdf', 'dot st', 'dot zw',
                'dot rfl diff', 'dot rfl spec', 'dot stR', 'dot rfl spec const', 'dependent AR', 'dependent GB',
                'dot product')
TEXGEN = {0x0000: 'disable', 0x2400: 'eye linear', 0x2401: 'object linear', 0x8511: 'normal map',
          0x8512: 'reflection map', 0x2402: 'sphere map'}


def input_name(byte, alpha):
    source = SOURCES[byte & 0xf]
    channel = '.a' if byte & 0x10 else ('.rgb' if not alpha else '.b')
    return '%s%s%s' % (MAPPINGS[byte >> 5 & 7], source, channel)


def icw(value, alpha):
    parts = [input_name(value >> shift & 0xff, alpha) for shift in (24, 16, 8, 0)]
    return 'A=%s B=%s C=%s D=%s' % tuple(parts)


def ocw(value):
    dst = lambda n: SOURCES[n] if n else 'discard'  # noqa: E731
    flags = []
    if value & 0x1000:
        flags.append('CD dot')
    if value & 0x2000:
        flags.append('AB dot')
    if value & 0x4000:
        flags.append('mux')
    # NV097_SET_COMBINER_COLOR / ALPHA_OCW: CD_DST bits 3:0, AB_DST 7:4, SUM_DST 11:8
    return 'AB->%s CD->%s sum->%s%s %s' % (dst(value >> 4 & 0xf), dst(value & 0xf), dst(value >> 8 & 0xf),
                                           SHIFTS[value >> 15 & 7], ' '.join(flags))


def final_cw(cw0, cw1):
    a, b, c, d = (input_name(cw0 >> s & 0xff, False) for s in (24, 16, 8, 0))
    e, f, g = (input_name(cw1 >> s & 0xff, False) for s in (24, 16, 8))
    return 'out.rgb = %s*%s + (1-%s)*%s + %s; E=%s F=%s; out.a = %s (cw1 low %#04x)' % (a, b, a, c, d, e, f, g,
                                                                                       cw1 & 0xff)


def floats(words):
    return ' '.join('%.3f' % struct.unpack('<f', struct.pack('<I', w))[0] for w in words)


def describe(method, values):
    """One NV097 method's words, or None when it is not one this probe reads."""
    lines = []
    for i, value in enumerate(values):
        m = method + 4 * i
        if 0x260 <= m < 0x280:
            lines.append('combiner %d alpha in : %s' % ((m - 0x260) // 4, icw(value, True)))
        elif 0xac0 <= m < 0xae0:
            lines.append('combiner %d colour in: %s' % ((m - 0xac0) // 4, icw(value, False)))
        elif 0xaa0 <= m < 0xac0:
            lines.append('combiner %d alpha out: %s' % ((m - 0xaa0) // 4, ocw(value)))
        elif 0x1e40 <= m < 0x1e60:
            lines.append('combiner %d colour out: %s' % ((m - 0x1e40) // 4, ocw(value)))
        elif 0xa60 <= m < 0xa80:
            lines.append('combiner %d factor 0 = %#010x' % ((m - 0xa60) // 4, value))
        elif 0xa80 <= m < 0xaa0:
            lines.append('combiner %d factor 1 = %#010x' % ((m - 0xa80) // 4, value))
        elif m == 0x1e60:
            lines.append('combiner control: %d stages, flags %#x' % (value & 0xff, value & ~0xff))
        elif m == 0x1e70:
            stages = [SHADER_MODES[value >> (5 * s) & 0x1f] if (value >> (5 * s) & 0x1f) < len(SHADER_MODES) else '?'
                      for s in range(4)]
            lines.append('texture shader program: %s' % ' / '.join(stages))
        elif 0x3c0 <= m < 0x400:
            lines.append('texgen stage %d %s = %s' % ((m - 0x3c0) // 16, 'STRQ'[(m - 0x3c0) // 4 % 4],
                                                      TEXGEN.get(value, hex(value))))
        elif m in (0x314, 0x3b8, 0x3bc, 0x298, 0x294):
            names = {0x314: 'lighting enable', 0x3b8: 'specular enable', 0x3bc: 'light enable mask',
                     0x298: 'colour material', 0x294: 'light control'}
            lines.append('%s = %#x' % (names[m], value))
    if method in (0x288,) and len(values) >= 2:
        lines.append('final combiner: ' + final_cw(values[0], values[1]))
    if method in (0xa10, 0x3a8, 0x1000, 0x100c) and len(values) >= 3:
        names = {0xa10: 'scene ambient', 0x3a8: 'material emission', 0x1000: 'light 0 ambient',
                 0x100c: 'light 0 diffuse'}
        lines.append('%s = %s' % (names[method], floats(values[:3])))
    return lines


TEXTURE_STAGE_FIELDS = ('offset', 'format', 'address', 'control 0', 'control 1', 'filter', '?', 'image rect')


def texture_stage(method, values):
    """NV097_SET_TEXTURE_* for stages 0..3 (0x1b00 + 0x40 x stage): offset, format, address, controls, filter, size."""
    lines = []
    for i, value in enumerate(values):
        m = method + 4 * i
        if 0x1b00 <= m < 0x1c00 and (m - 0x1b00) % 0x40 < 0x20:
            stage, field = (m - 0x1b00) // 0x40, (m - 0x1b00) % 0x40 // 4
            text = '%#010x' % value
            if field == 7:
                text = '%d x %d' % (value >> 16, value & 0xffff)
            lines.append('texture stage %d %s = %s' % (stage, TEXTURE_STAGE_FIELDS[field], text))
    return lines


class ConstantLoad:
    """NV097_SET_TRANSFORM_CONSTANT_LOAD (0x1ea4) picks the register the following constants (0x0b80..) fill;
    printed as the program's c registers (load index - 96: D3D's c0 is register 96)."""

    def __init__(self):
        self.register = 0

    def describe(self, method, values):
        if method == 0x1ea4:
            self.register = values[0]
            return []
        if 0xb80 <= method < 0xc00:
            lines = []
            for start in range(0, len(values) - 3, 4):
                lines.append('c%d = %s' % (self.register - 96, floats(values[start:start + 4])))
                self.register += 1
            return lines
        return []


def decode(data):
    words = struct.unpack('<%dI' % (len(data) // 4), data[:len(data) // 4 * 4])
    i, out = 0, []
    constants = ConstantLoad()
    while i < len(words):
        header = words[i]
        count, method, non_increasing = header >> 18 & 0x7ff, header & 0x1ffc, header & 0x40000000
        if header & 0x20000003 or count == 0:   # a jump / call, or not a method header: stop
            out.append('(stop at word %d: %#010x)' % (i, header))
            break
        values = list(words[i + 1:i + 1 + count])
        if non_increasing:
            values = values[:1]
        out.extend(describe(method, values))
        out.extend(texture_stage(method, values))
        out.extend(constants.describe(method, values))
        if method == 0x0300:
            out.append('alpha test enable = %d' % values[0])
        elif method == 0x0304:
            out.append('blend enable = %d' % values[0])
        i += 1 + count
    return out


def step_off(gdb, addr):
    """Leave the breakpoint just hit (continuing from it would hit it again at once): remove, single-step, re-arm."""
    gdb.cmd('z0,%x,1' % addr)
    gdb.cmd('s', 5)
    gdb.cmd('Z0,%x,1' % addr)


def wait_for_mesh(gdb, addr, verts):
    """Runs to the draw point until the mesh drawn there has the vertex count; the breakpoint is removed after."""
    if gdb.cmd('Z0,%x,1' % addr) != 'OK':
        raise SystemExit('breakpoint failed at %#x' % addr)
    try:
        while True:
            gdb.cont()
            gdb._recv(30)
            mesh = gdb.regs()['ebx']
            if struct.unpack('<I', gdb.read(mesh + 8, 4))[0] == verts:
                return mesh
            step_off(gdb, addr)
    finally:
        gdb.cmd('z0,%x,1' % addr)


def run_to(gdb, addr):
    """Continues to addr once (the next time it runs): one single step first, as a continue from the spot of the
    breakpoint just hit and removed returns at once in xemu's stub."""
    gdb.cmd('s', 5)
    if gdb.cmd('Z0,%x,1' % addr) != 'OK':
        raise SystemExit('breakpoint failed at %#x' % addr)
    try:
        gdb.cont()
        gdb._recv(10)
    finally:
        gdb.cmd('z0,%x,1' % addr)
    if gdb.regs()['eip'] != addr:
        raise SystemExit('stopped at %#x, not %#x' % (gdb.regs()['eip'], addr))


def put_pointer(gdb):
    device = struct.unpack('<I', gdb.read(DEVICE, 4))[0]
    return struct.unpack('<I', gdb.read(device, 4))[0]


FINAL_COMBINER_HEADER = 2 << 18 | 0x288   # NV097_SET_COMBINER_SPECULAR_FOG_CW0 + CW1, subchannel 0
BACK_SCAN_BYTES = 0x40000


def last_final_combiner(gdb, before):
    """The final combiner state is only sent when it changes, so not by every draw: the last write of it in the
    commands before this draw (a header word matched backwards; a value word could match it too, so it is a guess
    when the ring has wrapped)."""
    start = max(before - BACK_SCAN_BYTES, 0x80000000)
    data = gdb.read(start, before - start)
    words = struct.unpack('<%dI' % (len(data) // 4), data)
    for i in range(len(words) - 3, -1, -1):
        if words[i] == FINAL_COMBINER_HEADER:
            return start + 4 * i, words[i + 1], words[i + 2]
    return None


def main():
    verts = int(sys.argv[1], 16) if len(sys.argv) > 1 else 0xf0
    start = int(sys.argv[2], 16) if len(sys.argv) > 2 else 0xb78c8
    end = int(sys.argv[3], 16) if len(sys.argv) > 3 else 0xb78ef
    gdb = Gdb()
    try:
        wait_for_mesh(gdb, start, verts)
        before = put_pointer(gdb)
        run_to(gdb, end)
        after = put_pointer(gdb)
        if not before < after < before + 0x10000:
            raise SystemExit('the pushbuffer wrapped or was kicked (%#x -> %#x): run again' % (before, after))
        data = gdb.read(before, after - before)
        print('mesh with %#x vertices: %d bytes written at %#x' % (verts, len(data), before))
        for line in decode(data):
            print('  ' + line)
        final = last_final_combiner(gdb, before)
        if final:
            print('  last final combiner before it (at %#x): %s' % (final[0], final_cw(final[1], final[2])))
        else:
            print('  no final combiner write in the %#x bytes before it' % BACK_SCAN_BYTES)
    finally:
        gdb.cont()


if __name__ == '__main__':
    main()
