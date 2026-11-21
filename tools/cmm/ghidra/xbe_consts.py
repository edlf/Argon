# Read constants referenced in Ghidra decompiler output straight from default.xbe.
# usage: python xbe_consts.py <decomp.c> [more...]   -> FLOAT_/DAT_/_DAT_ name = value
import re, struct, sys, os
XBE = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', 'game_data', 'default.xbe')
d = open(XBE, 'rb').read()
base = struct.unpack_from('<I', d, 0x104)[0]
nsec, secp = struct.unpack_from('<II', d, 0x11c)
secs = []
for i in range(nsec):
    fl, va, vs, raw, rs = struct.unpack_from('<IIIII', d, secp - base + i * 56)
    secs.append((va, vs, raw, rs))
def rd(va, n):
    for va0, vs, raw, rs in secs:
        if va0 <= va and va + n <= va0 + rs: return d[raw + va - va0: raw + va - va0 + n]
    return None   # bss / not in file (runtime variable)
def value(name):
    a = int(re.search(r'([0-9a-fA-F]{6,8})$', name).group(1), 16)
    b = rd(a, 8)
    if b is None: return 'runtime (bss)'
    f = struct.unpack_from('<f', b)[0]; dd = struct.unpack_from('<d', b)[0]; i = struct.unpack_from('<i', b)[0]
    if name.startswith('DOUBLE'): return 'double %r' % dd
    return 'f %r  (i %d)' % (f, i)
if __name__ == '__main__':
    names = set()
    for p in sys.argv[1:]:
        names |= set(re.findall(r'\b(?:FLOAT|_DAT|DAT|DOUBLE)_00[0-9a-f]{6}\b', open(p).read()))
    for n in sorted(names, key=lambda s: s[-8:]):
        print('%-16s %s' % (n, value(n)))
