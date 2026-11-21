"""Disassemble Xbox (NV2A) compiled vertex shaders (.xvu) into D3D-style assembly.
usage: python -I xvu.py shaders\\water.xvu [...]
File: u16 type (0x2078), u16 instruction count, then 16-byte instructions (dword 0 unused).
Field layout as in xemu's nv2a vsh.c. Constants are printed as c[n] with n = raw - 96 (D3D numbering).
"""
import struct, sys

MAC = ['nop', 'mov', 'mul', 'add', 'mad', 'dp3', 'dph', 'dp4', 'dst', 'min', 'max', 'slt', 'sge', 'arl']
ILU = ['nop', 'mov', 'rcp', 'rcc', 'rsq', 'exp', 'log', 'lit']
NARGS_MAC = {'mov': 'A', 'mul': 'AB', 'add': 'AC', 'mad': 'ABC', 'dp3': 'AB', 'dph': 'AB', 'dp4': 'AB',
             'dst': 'AB', 'min': 'AB', 'max': 'AB', 'slt': 'AB', 'sge': 'AB', 'arl': 'A'}
OUTS = {0: 'oPos', 3: 'oD0', 4: 'oD1', 5: 'oFog', 6: 'oPts', 7: 'oB0', 8: 'oB1',
        9: 'oT0', 10: 'oT1', 11: 'oT2', 12: 'oT3'}
VIN = ['v%d' % i for i in range(16)]


def fld(w, d, s, n):
    return (w[d] >> s) & ((1 << n) - 1)


def swz(x, y, z, w):
    s = ''.join('xyzw'[c] for c in (x, y, z, w))
    if s == 'xyzw':
        return ''
    if len(set(s)) == 1:
        return '.' + s[0]
    return '.' + s


def mask(m):
    s = ''.join(c for i, c in enumerate('xyzw') if m & (8 >> i))
    return '' if s == 'xyzw' else '.' + s


def decode(w):
    ilu = fld(w, 1, 25, 3); mac = fld(w, 1, 21, 4)
    const = fld(w, 1, 13, 8); vin = fld(w, 1, 9, 4)
    a0x = fld(w, 3, 1, 1)

    def src(neg, sx, sy, sz, sw, reg, mux):
        if mux == 1: r = 'r%d' % reg
        elif mux == 2: r = VIN[vin]
        elif mux == 3: r = 'c[a0.x+%d]' % (const - 96) if a0x else 'c[%d]' % (const - 96)
        else: r = '?'
        return ('-' if neg else '') + r + swz(sx, sy, sz, sw)

    A = src(fld(w, 1, 8, 1), fld(w, 1, 6, 2), fld(w, 1, 4, 2), fld(w, 1, 2, 2), fld(w, 1, 0, 2),
            fld(w, 2, 28, 4), fld(w, 2, 26, 2))
    B = src(fld(w, 2, 25, 1), fld(w, 2, 23, 2), fld(w, 2, 21, 2), fld(w, 2, 19, 2), fld(w, 2, 17, 2),
            fld(w, 2, 13, 4), fld(w, 2, 11, 2))
    C = src(fld(w, 2, 10, 1), fld(w, 2, 8, 2), fld(w, 2, 6, 2), fld(w, 2, 4, 2), fld(w, 2, 2, 2),
            (fld(w, 2, 0, 2) << 2) | fld(w, 3, 30, 2), fld(w, 3, 28, 2))
    mac_mask = fld(w, 3, 24, 4); out_r = fld(w, 3, 20, 4); ilu_mask = fld(w, 3, 16, 4)
    o_mask = fld(w, 3, 12, 4); orb = fld(w, 3, 11, 1); addr = fld(w, 3, 3, 8); omux = fld(w, 3, 2, 1)
    final = fld(w, 3, 0, 1)
    args = {'A': A, 'B': B, 'C': C}
    oname = (OUTS.get(addr, 'o?%d' % addr) if orb else 'c[%d]' % (addr - 96))
    lines = []
    if mac:
        op = MAC[mac] if mac < len(MAC) else 'mac%d' % mac
        srcs = ', '.join(args[k] for k in NARGS_MAC.get(op, 'A'))
        dsts = []
        if mac_mask: dsts.append('r%d%s' % (out_r, mask(mac_mask)))
        if o_mask and omux == 0: dsts.append(oname + mask(o_mask))
        if op == 'arl': dsts = ['a0.x']
        for dd in dsts or ['(none)']:
            lines.append('%-4s %s, %s' % (op, dd, srcs))
    if ilu:
        op = ILU[ilu]
        dsts = []
        if ilu_mask: dsts.append('r%d%s' % (1 if mac else out_r, mask(ilu_mask)))
        if o_mask and omux == 1: dsts.append(oname + mask(o_mask))
        for dd in dsts or ['(none)']:
            lines.append('%-4s %s, %s' % (op, dd, C))
    if not lines: lines.append('nop')
    return lines, final


def disasm(path):
    d = open(path, 'rb').read()
    typ, n = struct.unpack_from('<HH', d, 0)
    print('; %s  type %#x  %d instructions' % (path, typ, n))
    for i in range(n):
        w = struct.unpack_from('<4I', d, 4 + i * 16)
        lines, final = decode(w)
        for j, l in enumerate(lines):
            print('%3s  %s' % (str(i) if j == 0 else '', l))
        if final:
            print('     ; end')
            break


if __name__ == '__main__':
    for p in sys.argv[1:]:
        disasm(p)
        print()
